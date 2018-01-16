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
#pragma once

#include <ns3/simulator.h>
#include <ns3/default-simulator-impl.h>
#include "concurrent-queue.h"

namespace ns3 {

class ConcurrentSimulatorImpl : public DefaultSimulatorImpl
{
public:
  /**
   * Get the registered TypeId for this class.
   * \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Constructor. */
  ConcurrentSimulatorImpl ();
  /** Destructor. */
  virtual ~ConcurrentSimulatorImpl () override;

  // Inherited from SimulatorImpl
  virtual void Destroy () override;
  virtual bool IsFinished (void) const override;
  virtual EventId Schedule (const Time &delay, EventImpl *event) override;
  virtual void ScheduleWithContext (uint32_t context, const Time &delay, EventImpl *event) override;
  virtual EventId ScheduleNow (EventImpl *event) override;
  virtual EventId ScheduleDestroy (EventImpl *event) override;
  virtual void Remove (const EventId &id) override;
  virtual void Run (void) override;

private:
  virtual void DoDispose (void) override;
  void ProcessOneEvent();
  void InsertEventInPaper (EventImpl *event, uint64_t ts,
                           uint32_t context, uint32_t uid);
  void InsertDestroyEventInStone ();
  void InsertRemoveEventInStone ();
  void InsertEventInStone ();
  void Sync ();

  bool m_running = false; //!< Simulator is running?

  moodycamel::ConcurrentQueue<Scheduler::Event> m_paperEvents; //!< Container for "paper" events
  moodycamel::ConcurrentQueue<EventId> m_paperEventsDestroy; //!< Container for "paper" destroy events
  moodycamel::ConcurrentQueue<EventId> m_paperEventsRemove; //!< Container for "paper" events to remove
};

} // namespace ns3
