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
#include "tcp-socket-state.h"

namespace ns3 {

StateTracedValues::StateTracedValues ()
  : m_cWnd (0),
    m_ssThresh (0),
    m_highTxMark (0),
    m_nextTxSequence (0)
{
}

StateTracedValues::StateTracedValues (const StateTracedValues &other)
  : m_cWnd (other.m_cWnd),
    m_ssThresh (other.m_ssThresh),
    m_highTxMark (other.m_highTxMark),
    m_nextTxSequence (other.m_nextTxSequence)
{
}

StateTracedValues::~StateTracedValues ()
{
  m_cWnd = 0;
  m_ssThresh = 0;
  m_highTxMark = 0;
  m_nextTxSequence = 0;
}

TypeId
TcpSocketState::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketState")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddConstructor <TcpSocketState> ()
    .AddTraceSource ("CongState",
                     "TCP Congestion machine state",
                     MakeTraceSourceAccessor (&TcpSocketState::m_congState),
                     "ns3::TracedValue::TcpCongStatesTracedValueCallback")
  ;
  return tid;
}

TcpSocketState::TcpSocketState (void)
  : Object (),
    m_initialCWnd (0),
    m_initialSsThresh (0),
    m_segmentSize (0),
    m_lastAckedSeq (0),
    m_congState (CA_OPEN)
{
}

TcpSocketState::TcpSocketState (const TcpSocketState &other)
  : Object (other),
    m_initialCWnd (other.m_initialCWnd),
    m_initialSsThresh (other.m_initialSsThresh),
    m_segmentSize (other.m_segmentSize),
    m_lastAckedSeq (other.m_lastAckedSeq),
    m_congState (other.m_congState)
{
}

const char* const
TcpSocketState::TcpCongStateName[TcpSocketState::CA_LAST_STATE] =
{
  "CA_OPEN", "CA_DISORDER", "CA_CWR", "CA_RECOVERY", "CA_LOSS"
};


} // namespace ns3
