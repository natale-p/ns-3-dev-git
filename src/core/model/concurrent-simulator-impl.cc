/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) Natale Patriciello <natale.patriciello@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "concurrent-simulator-impl.h"
#include "ns3/log.h"
#include <stlab/concurrency/utility.hpp>

namespace ns3 {

// Note:  Logging in this file is largely avoided due to the
// number of calls that are made to these functions and the possibility
// of causing recursions leading to stack overflow. Moreover, logging
// in methods that can be called concurrently is avoided because of
// the contention in the output object.

NS_LOG_COMPONENT_DEFINE ("ConcurrentSimulatorImpl");
NS_OBJECT_ENSURE_REGISTERED (ConcurrentSimulatorImpl);

TypeId
ConcurrentSimulatorImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConcurrentSimulatorImpl")
    .SetParent<DefaultSimulatorImpl> ()
    .SetGroupName ("Core")
    .AddConstructor<ConcurrentSimulatorImpl> ()
  ;
  return tid;
}

// Not concurrent. Only one instance of this object is created
ConcurrentSimulatorImpl::ConcurrentSimulatorImpl ()
  : DefaultSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
}

// Not concurrent.
ConcurrentSimulatorImpl::~ConcurrentSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
}

// Called only one time from the main thread.
void
ConcurrentSimulatorImpl::Run (void)
{
  NS_LOG_FUNCTION (this);
  m_running = true;
  m_stop = false;
  m_main = SystemThread::Self();

  Sync ();
  ProcessEventsWithContext ();

  while (!m_events->IsEmpty () && !m_stop)
    {
      ProcessOneEvent ();
    }

  m_running = false;
}

void
ConcurrentSimulatorImpl::ProcessOneEvent (void)
{
  Scheduler::Event next = m_events->RemoveNext ();

  NS_ASSERT (next.key.m_ts >= m_currentTs);
  m_unscheduledEvents--;

  NS_LOG_LOGIC ("handle " << next.key.m_ts);
  m_currentTs = next.key.m_ts;
  m_currentContext = next.key.m_context;
  m_currentUid = next.key.m_uid;
  next.impl->Invoke ();
  next.impl->Unref();
  ProcessEventsWithContext ();
  Sync ();
}

// Called at distruction time from the main thread
void
ConcurrentSimulatorImpl::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  Sync ();

  SimulatorImpl::DoDispose ();
}

// Called from the main thread, everytime
void
ConcurrentSimulatorImpl::Sync()
{
  NS_LOG_FUNCTION (this);

  InsertDestroyEventInStone ();
  InsertEventInStone ();
  InsertRemoveEventInStone ();
}

// Possibility of concurrency here. Try to be fast, no logging.
void
ConcurrentSimulatorImpl::InsertEventInPaper(EventImpl *event, uint64_t ts,
                                            uint32_t context, uint32_t uid)
{
  // Create the Scheduler::Event
  Scheduler::Event ev;
  ev.impl = event;
  ev.key.m_ts = ts;
  ev.key.m_context = context;
  ev.key.m_uid = uid;

  // Insert it into paper
  m_paperEvents.enqueue(ev);
}

void
ConcurrentSimulatorImpl::InsertEventInStone ()
{
  NS_LOG_FUNCTION (this);

  while (m_paperEvents.size_approx() != 0)
    {
      Scheduler::Event event;
      bool res;
      res = m_paperEvents.try_dequeue (event);
      NS_ASSERT(res);
      DefaultSimulatorImpl::Insert (event);
      m_unscheduledEvents++;
    }
}

void
ConcurrentSimulatorImpl::InsertRemoveEventInStone()
{
  NS_LOG_FUNCTION (this);

  while (m_paperEventsRemove.size_approx() != 0)
    {
      EventId event;
      bool res;
      res = m_paperEventsRemove.try_dequeue(event);
      NS_ASSERT (res);
      DefaultSimulatorImpl::Remove (event);
    }
}

EventId
ConcurrentSimulatorImpl::Schedule (Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << delay.GetTimeStep () << event);

  Time tAbsolute = delay + TimeStep (m_currentTs);
  uint64_t ts = static_cast<uint64_t> (tAbsolute.GetTimeStep ());
  uint32_t context = GetContext ();

  // Atomically generate the new uid. Don't break that instruction!
  uint32_t uid = m_uid.fetch_add(1);

  NS_ASSERT (tAbsolute.IsPositive ());
  NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));

  InsertEventInPaper (event, ts, context, uid);

  return EventId (event, ts, context, uid);
}

void
ConcurrentSimulatorImpl::ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << context << delay.GetTimeStep () << event);

  Time tAbsolute = delay + TimeStep (m_currentTs);
  uint64_t ts = static_cast<uint64_t> (tAbsolute.GetTimeStep ());

  // Atomically generate the new uid. Don't break that instruction!
  uint32_t uid = m_uid.fetch_add(1);

  NS_ASSERT (tAbsolute.IsPositive ());
  NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));

  InsertEventInPaper(event, ts, context, uid);
}

EventId
ConcurrentSimulatorImpl::ScheduleNow (EventImpl *event)
{
  return Schedule (Time (0), event);
}

EventId
ConcurrentSimulatorImpl::ScheduleDestroy (EventImpl *event)
{
  EventId id (Ptr<EventImpl> (event, false), m_currentTs, 0xffffffff, 2);
  m_paperEventsDestroy.enqueue(id);
  // Atomically generate the new uid. Don't break that instruction!
  m_uid.fetch_add(1);
  return id;
}

void
ConcurrentSimulatorImpl::InsertDestroyEventInStone()
{
  NS_LOG_FUNCTION (this);

  while (m_paperEventsDestroy.size_approx() != 0)
    {
      EventId event;
      bool res;
      res = m_paperEventsDestroy.try_dequeue (event);

      NS_ASSERT(res);
      NS_UNUSED(res);

      DefaultSimulatorImpl::InsertDestroy(event);
    }
}

void
ConcurrentSimulatorImpl::Destroy ()
{
  if (! DefaultSimulatorImpl::IsFinished())
    {
      NS_FATAL_ERROR("Can't destroy simulator if it's not finished.");
    }

  Sync ();

  DefaultSimulatorImpl::Destroy ();
}

void
ConcurrentSimulatorImpl::Remove (const EventId &id)
{
  m_paperEventsRemove.enqueue(id);
}

bool
ConcurrentSimulatorImpl::IsFinished () const
{
  return DefaultSimulatorImpl::IsFinished () &&
      m_paperEvents.size_approx () == 0 &&
      m_paperEventsRemove.size_approx () == 0 &&
      m_running == false;
}

} //namespace ns3
