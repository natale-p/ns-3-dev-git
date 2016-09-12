/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 * Copyright (c) 2010 Adrian Sai-wah Tam
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
 * Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */
#ifndef TCP_RTT_HISTORY_H
#define TCP_RTT_HISTORY_H

#include "ns3/sequence-number.h"
#include "ns3/nstime.h"
#include <deque>

namespace ns3 {

/**
 * \ingroup tcp
 *
 * \brief Helper class to store RTT measurements
 */
class RttHistory
{
public:
  /**
   * \brief Constructor - builds an RttHistory with the given parameters
   * \param s First sequence number in packet sent
   * \param c Number of bytes sent
   * \param t Time this one was sent
   */
  RttHistory (SequenceNumber32 s, uint32_t c, Time t);
  /**
   * \brief Copy constructor
   * \param h the object to copy
   */
  RttHistory (const RttHistory& h); // Copy constructor
public:
  SequenceNumber32  seq;  //!< First sequence number in packet sent
  uint32_t        count;  //!< Number of bytes sent
  Time            time;   //!< Time this one was sent
  bool            retx;   //!< True if this has been retransmitted
};

/// Container for RttHistory objects
typedef std::deque<RttHistory> RttHistory_t;

} // namespace ns3
#endif // TCP_RTT_HISTORY_H
