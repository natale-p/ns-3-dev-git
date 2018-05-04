/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
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

#include "ns3/sequence-number.h"
#include "ns3/ptr.h"
#include "ns3/nstime.h"

namespace ns3 {

class Packet;

/**
 * \ingroup tcp
 *
 * \brief Item that encloses the application packet and some flags for it
 */
class TcpTxItem
{
public:
  // Default constructor, copy-constructor, destructor

  /**
   * \brief Print the time
   * \param os ostream
   */
  void Print (std::ostream &os) const;

  /**
   * \brief Get the size in the sequence number space
   *
   * \return 1 if the packet size is 0 or there's no packet, otherwise the size of the packet
   */
  uint32_t GetSeqSize (void) const;

  SequenceNumber32 m_startSeq {0};   //!< Sequence number of the item (if transmitted)
  Ptr<Packet> m_packet {nullptr};    //!< Application packet (can be null)
  bool m_lost          {false};      //!< Indicates if the segment has been lost (RTO)
  bool m_retrans       {false};      //!< Indicates if the segment is retransmitted
  Time m_lastSent      {Time::Min()};//!< Timestamp of the time at which the segment has been sent last time
  bool m_sacked        {false};      //!< Indicates if the segment has been SACKed

  // For Rate Sample. Each value is the value at the time the packet was sent
  Time m_firstTxStamp  {Seconds (0)};//!< Start of send pipeline phase
  Time m_deliveredStamp{Seconds (0)};//!< When we reached the "delivered" count
  uint64_t m_delivered {0};          //!< Bytes S/ACKed, incl retrans
  bool m_isAppLimited  {false};      //!< Connection is app limited?
};

} //namespace ns3
