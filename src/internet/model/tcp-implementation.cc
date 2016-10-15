/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Natale Patriciello <natale.patriciello@gmail.com>
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
#include "tcp-implementation.h"
#include "ns3/packet.h"
#include "ns3/tcp-socket-base.h"

namespace ns3 {

TypeId
TcpImplementation::GetInstanceTypeId () const
{
  return TcpImplementation::GetTypeId ();
}
TypeId
TcpImplementation::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpImplementation")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}
TcpImplementation::TcpImplementation ()
{
}
TcpImplementation::~TcpImplementation ()
{
}

Time
TcpTracedValues::GetRto () const
{
  NS_ASSERT (m_rto != 0);
  return *m_rto;
}
void
TcpTracedValues::SetRtoPointer (TracedValue<Time> *rto)
{
  NS_ASSERT (rto != 0);
  m_rto = rto;
}
void
TcpTracedValues::SetRto (const Time &rto)
{
  NS_ASSERT (m_rto != 0);
  *m_rto = rto;
}
Time
TcpTracedValues::GetLastRtt () const
{
  NS_ASSERT (m_lastRtt != 0);
  return *m_lastRtt;
}
void
TcpTracedValues::SetLastRttPointer (TracedValue<Time> *lastRtt)
{
  NS_ASSERT (lastRtt != 0);
  m_lastRtt = lastRtt;
}
void
TcpTracedValues::SetLastRtt (const Time &lastRtt)
{
  NS_ASSERT (m_lastRtt != 0);
  *m_lastRtt = lastRtt;
}
uint32_t
TcpTracedValues::GetRWnd () const
{
  NS_ASSERT (m_rWnd != 0);
  return *m_rWnd;
}
void
TcpTracedValues::SetRWndPointer (TracedValue<uint32_t> *rWnd)
{
  NS_ASSERT (rWnd != 0);
  m_rWnd = rWnd;
}
void
TcpTracedValues::SetRwnd (uint32_t rWnd)
{
  NS_ASSERT (m_rWnd != 0);
  *m_rWnd = rWnd;
}
SequenceNumber32
TcpTracedValues::GetHighRxMark () const
{
  NS_ASSERT (m_highRxMark != 0);
  return *m_highRxMark;
}
void
TcpTracedValues::SetHighRxMarkPointer (TracedValue<SequenceNumber32> *highRxMark)
{
  NS_ASSERT (highRxMark != 0);
  m_highRxMark = highRxMark;
}
void
TcpTracedValues::SetHighRxMark (const SequenceNumber32 &highRxMark)
{
  NS_ASSERT (m_highRxMark != 0);
  *m_highRxMark = highRxMark;
}
SequenceNumber32
TcpTracedValues::GetHighRxAckMark () const
{
  NS_ASSERT (m_highRxAckMark != 0);
  return *m_highRxAckMark;
}
void
TcpTracedValues::SetHighRxAckMarkPointer (TracedValue<SequenceNumber32> *highRxAckMark)
{
  NS_ASSERT (highRxAckMark != 0);
  m_highRxAckMark = highRxAckMark;
}
void
TcpTracedValues::SetHighRxAckMark (const SequenceNumber32 &highRxAckMark)
{
  NS_ASSERT (m_highRxAckMark != 0);
  *m_highRxAckMark = highRxAckMark;
}
uint32_t
TcpTracedValues::GetBytesInFlight () const
{
  NS_ASSERT (m_bytesInFlight != 0);
  return *m_bytesInFlight;
}
void
TcpTracedValues::SetBytesInFlightPointer (TracedValue<uint32_t> *bytesInFlight)
{
  NS_ASSERT (bytesInFlight != 0);
  m_bytesInFlight = bytesInFlight;
}

void
TcpTracedValues::SetBytesInFlight (uint32_t bytesInFlight)
{
  NS_ASSERT (m_bytesInFlight != 0);
  *m_bytesInFlight = bytesInFlight;
}
uint32_t
TcpTracedValues::GetCWnd () const
{
  NS_ASSERT (m_cWnd != 0);
  return *m_cWnd;
}
void
TcpTracedValues::SetCWndPointer (TracedValue<uint32_t> *cWnd)
{
  NS_ASSERT (cWnd != 0);
  m_cWnd = cWnd;
}
void
TcpTracedValues::SetCwnd (uint32_t cWnd)
{
  NS_ASSERT (m_cWnd != 0);
  *m_cWnd = cWnd;
}
uint32_t
TcpTracedValues::GetSSThresh () const
{
  NS_ASSERT (m_ssThresh != 0);
  return *m_ssThresh;
}
void
TcpTracedValues::SetSSThreshPointer (TracedValue<uint32_t> *ssThresh)
{
  NS_ASSERT (ssThresh != 0);
  m_ssThresh = ssThresh;
}
void
TcpTracedValues::SetSSThresh (uint32_t ssThresh)
{
  NS_ASSERT (m_ssThresh != 0);
  *m_ssThresh = ssThresh;
}
SequenceNumber32
TcpTracedValues::GetHighTxMark () const
{
  NS_ASSERT (m_highTxMark != 0);
  return *m_highTxMark;
}
void
TcpTracedValues::SetHighTxMarkPointer (TracedValue<SequenceNumber32> *highTxMark)
{
  NS_ASSERT (highTxMark != 0);
  m_highTxMark = highTxMark;
}
void
TcpTracedValues::SetHighTxMark (const SequenceNumber32 &highTxMark)
{
  NS_ASSERT (m_highTxMark != 0);
  *m_highTxMark = highTxMark;
}
SequenceNumber32
TcpTracedValues::GetNextTxSequence () const
{
  NS_ASSERT (m_nextTxSequence != 0);
  return *m_nextTxSequence;
}
void
TcpTracedValues::SetNextTxSequencePointer (TracedValue<SequenceNumber32> *nextTxSequence)
{
  NS_ASSERT (nextTxSequence != 0);
  m_nextTxSequence = nextTxSequence;
}
void
TcpTracedValues::SetNextTxSequence (const SequenceNumber32 &nextTxSequence)
{
  NS_ASSERT (m_nextTxSequence != 0);
  *m_nextTxSequence = nextTxSequence;
}
void
TcpTracedValues::SetTxTracePointer (TcpPktTraceCb *txTrace)
{
  NS_ASSERT (txTrace != 0);
  m_txTrace = txTrace;
}
void
TcpTracedValues::SetRxTracePointer (TcpPktTraceCb *rxTrace)
{
  NS_ASSERT (rxTrace != 0);
  m_txTrace = rxTrace;
}
void
TcpTracedValues::TxTrace (Ptr<const Packet> p, const TcpHeader& h, Ptr<const TcpSocketBase> socket)
{
  NS_ASSERT(m_txTrace != 0);
  (*m_txTrace) (p, h, socket);
}
void
TcpTracedValues::RxTrace (Ptr<const Packet> p, const TcpHeader& h, Ptr<const TcpSocketBase> socket)
{
  NS_ASSERT(m_rxTrace != 0);
  (*m_rxTrace) (p, h, socket);
}
} // namespace ns3
