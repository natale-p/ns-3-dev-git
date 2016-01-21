/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010-2015 Adrian Sai-wah Tam
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
 * Original author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#include <algorithm>

#include "ns3/packet.h"
#include "ns3/log.h"
#include "ns3/abort.h"

#include "tcp-tx-buffer.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpTxBuffer");

TcpTxItem::TcpTxItem ()
  : m_packet (0),
    m_retrans (false),
    m_lastSent (Time::Min ())
{
}

TcpTxItem::TcpTxItem (const TcpTxItem &other)
  : m_packet (other.m_packet),
    m_retrans (other.m_retrans),
    m_lastSent (other.m_lastSent)
{
}

void
TcpTxItem::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this);
  os << "pkt pointer: " << m_packet;

  if (m_retrans)
    {
      os << "[retrans]";
    }
  os << ", last sent: " << m_lastSent;
}

NS_OBJECT_ENSURE_REGISTERED (TcpTxBuffer);

TypeId
TcpTxBuffer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTxBuffer")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpTxBuffer> ()
    .AddTraceSource ("UnackSequence",
                     "First unacknowledged sequence number (SND.UNA)",
                     MakeTraceSourceAccessor (&TcpTxBuffer::m_firstByteSeq),
                     "ns3::SequenceNumber32TracedValueCallback")
  ;
  return tid;
}

/* A user is supposed to create a TcpSocket through a factory. In TcpSocket,
 * there are attributes SndBufSize and RcvBufSize to control the default Tx and
 * Rx window sizes respectively, with default of 128 KiByte. The attribute
 * SndBufSize is passed to TcpTxBuffer by TcpSocketBase::SetSndBufSize() and in
 * turn, TcpTxBuffer:SetMaxBufferSize(). Therefore, the m_maxBuffer value
 * initialized below is insignificant.
 */
TcpTxBuffer::TcpTxBuffer (uint32_t n)
  : m_maxBuffer (32768), m_size (0), m_sentSize (0), m_firstByteSeq (n)
{
}

TcpTxBuffer::~TcpTxBuffer (void)
{
}

SequenceNumber32
TcpTxBuffer::HeadSequence (void) const
{
  return m_firstByteSeq;
}

SequenceNumber32
TcpTxBuffer::TailSequence (void) const
{
  return m_firstByteSeq + SequenceNumber32 (m_size);
}

uint32_t
TcpTxBuffer::Size (void) const
{
  return m_size;
}

uint32_t
TcpTxBuffer::MaxBufferSize (void) const
{
  return m_maxBuffer;
}

void
TcpTxBuffer::SetMaxBufferSize (uint32_t n)
{
  m_maxBuffer = n;
}

uint32_t
TcpTxBuffer::Available (void) const
{
  return m_maxBuffer - m_size;
}

void
TcpTxBuffer::SetHeadSequence (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  m_firstByteSeq = seq;
}

bool
TcpTxBuffer::Add (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  NS_LOG_INFO ("Try to append " << p->GetSize () << " bytes to window starting at "
                                << m_firstByteSeq << ", availSize=" << Available ());
  if (p->GetSize () <= Available ())
    {
      if (p->GetSize () > 0)
        {
          m_appList.insert (m_appList.end (), p);
          m_size += p->GetSize ();

          NS_LOG_INFO ("Updated size=" << m_size << ", lastSeq=" <<
                       m_firstByteSeq + SequenceNumber32 (m_size));
        }
      return true;
    }
  NS_LOG_WARN ("Rejected. Not enough room to buffer packet.");
  return false;
}

uint32_t
TcpTxBuffer::SizeFromSequence (const SequenceNumber32& seq) const
{
  NS_LOG_FUNCTION (this << seq);
  // Sequence of last byte in buffer
  SequenceNumber32 lastSeq = TailSequence ();

  if (lastSeq >= seq)
    {
      return lastSeq - seq;
    }

  NS_LOG_ERROR ("Requested a sequence beyond our space (" << seq << " > " << lastSeq <<
                "). Returning 0 for convenience.");
  return 0;
}

Ptr<Packet>
TcpTxBuffer::CopyFromSequence (uint32_t numBytes, const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (*this << numBytes << seq);

  if (m_firstByteSeq > seq)
    {
      NS_LOG_ERROR ("Requested a sequence number which is not in the buffer anymore");
      return Create<Packet> ();
    }

  // Real size to extract. Insure not beyond end of data
  uint32_t s = std::min (numBytes, SizeFromSequence (seq));

  if (s == 0)
    {
      return Create<Packet> ();
    }

  Ptr<Packet> outPacket = 0;

  if (m_firstByteSeq + m_sentSize >= seq + s)
    {
      // already sent this block completely
      outPacket = GetTransmittedSegment (s, seq);
      NS_LOG_DEBUG ("Retransmitting [" << seq << ";" << seq + s << "|" << s <<
                    "] from " << *this);
    }
  else if (m_firstByteSeq + m_sentSize <= seq)
    {
      NS_ABORT_MSG_UNLESS (m_firstByteSeq + m_sentSize == seq,
                           "Requesting a piece of new data with an hole");

      // this is the first time we transmit this block
      outPacket = GetNewSegment (s);

      NS_LOG_DEBUG ("New segment [" << seq << ";" << seq + s << "|" << s <<
                    "] from " << *this);
    }
  else if (m_firstByteSeq + m_sentSize > seq && m_firstByteSeq + m_sentSize < seq + s)
    {
      // Partial: a part is retransmission, the remaining data is new

      // Take the new data and move it into sent list
      uint32_t amount = seq + s - m_firstByteSeq.Get () - m_sentSize;
      NS_LOG_DEBUG ("Moving segment [" << m_firstByteSeq + m_sentSize << ";" <<
                    m_firstByteSeq + m_sentSize + amount <<"|" << amount <<
                    "] from " << *this);
      GetNewSegment (amount);

      // Now get outPacket from the sent list (there will be a merge)
      outPacket = GetTransmittedSegment (s, seq);
      NS_LOG_DEBUG ("Retransmitting [" << seq << ";" << seq + s << "|" << s <<
                    "] from " << *this);
    }

  NS_ASSERT (outPacket != 0);
  NS_ASSERT (outPacket->GetSize () == s);
  return outPacket->Copy ();
}

Ptr<Packet>
TcpTxBuffer::GetNewSegment (uint32_t numBytes)
{
  NS_LOG_FUNCTION (this << numBytes);

  SequenceNumber32 startOfAppList = m_firstByteSeq + m_sentSize;

  Ptr<Packet> p = GetPacketFromList (m_appList, startOfAppList,
                                     numBytes, startOfAppList);

  // Move p from AppList to SentList

  PacketList::iterator it = std::find (m_appList.begin (), m_appList.end (), p);
  NS_ASSERT (it != m_appList.end ());
  m_appList.erase (it);
  m_sentList.insert (m_sentList.end (), p);
  m_sentSize += p->GetSize ();

  return p;
}

Ptr<Packet>
TcpTxBuffer::GetTransmittedSegment (uint32_t numBytes, const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this << numBytes << seq);
  NS_ASSERT (seq >= m_firstByteSeq);
  NS_ASSERT (numBytes <= m_sentSize);

  return GetPacketFromList (m_sentList, m_firstByteSeq, numBytes, seq);
}

Ptr<Packet>
TcpTxBuffer::GetPacketFromList (PacketList &list, const SequenceNumber32 &listStartFrom,
                                uint32_t numBytes, const SequenceNumber32 &seq) const
{
  NS_LOG_FUNCTION (this << numBytes << seq);

  /*
   * Our possibilites are sketched out in the following:
   *
   *                    |------|     |----|     |----|
   * GetList (m_data) = |      | --> |    | --> |    |
   *                    |------|     |----|     |----|
   *
   *                    ^ ^ ^  ^
   *                    | | |  |         (1)
   *                  seq | |  numBytes
   *                      | |
   *                      | |
   *                    seq numBytes     (2)
   *
   * (1) seq and numBytes are the boundary of some packet
   * (2) seq and numBytes are not the boundary of some packet
   *
   * We can have mixed case (e.g. seq over the boundary while numBytes not).
   *
   * If we discover that we are in (2) or in a mixed case, we split
   * packets accordingly to the requested bounds and re-run the function.
   *
   * In (1), things are pretty easy, it's just a matter of walking the list and
   * defragment packets, if needed (e.g. seq is the beginning of the first packet
   * while maxBytes is the end of some packet next in the list).
   */

  Ptr<Packet> outPacket = 0;
  PacketList::iterator it = list.begin ();
  SequenceNumber32 beginOfCurrentPacket = listStartFrom;

  while (it != list.end ())
    {
      Ptr<Packet> current = (*it);

      // The objective of this snippet is to find (or to create) the packet
      // that begin with the sequence seq

      if (seq < beginOfCurrentPacket + current->GetSize ())
        {
          // seq is inside the current packet
          if (seq == beginOfCurrentPacket)
            {
              // seq is the beginning of the current packet. Hurray!
              outPacket = current;
              NS_LOG_INFO ("Current packet starts at seq " << seq <<
                           " ends at " << seq + outPacket->GetSize ());
            }
          else
            {
              // seq is inside the current packet but seq is not the beginning,
              // it's somewhere in the middle. Just fragment the beginning and
              // start again.
              NS_LOG_INFO ("we are at " << beginOfCurrentPacket <<
                           " searching for " << seq <<
                           " and now we recurse because packet ends at "
                                        << beginOfCurrentPacket + current->GetSize ());
              Ptr<Packet> p = current->CreateFragment (0, seq - beginOfCurrentPacket);
              current->RemoveAtStart (seq - beginOfCurrentPacket);
              list.insert (it, p);
              return GetPacketFromList (list, listStartFrom, numBytes, seq);
            }
        }
      else
        {
          // Walk the list, the current packet does not contain seq
          beginOfCurrentPacket += current->GetSize ();
          it++;
          continue;
        }

      NS_ASSERT (outPacket != 0);

      // The objective of this snippet is to find (or to create) the packet
      // that ends after numBytes bytes. We are sure that outPacket starts
      // at seq.

      if (seq + numBytes <= beginOfCurrentPacket + current->GetSize ())
        {
          // the end boundary is inside the current packet
          if (numBytes == current->GetSize ())
            {
              // the end boundary is exactly the end of the current packet. Hurray!
              if (current == outPacket)
                {
                  // A perfect match!
                  return outPacket;
                }
              else
                {
                  // the end is exactly the end of current packet, but
                  // current > outPacket in the list. Merge current with the
                  // previous, and recurse.
                  NS_ASSERT (it != list.begin ());
                  Ptr<Packet> previous = *(it--);
                  previous->AddAtEnd (current);
                  list.erase (it);
                  return GetPacketFromList (list, listStartFrom, numBytes, seq);
                }
            }
          else if (numBytes < current->GetSize ())
            {
              // the end is inside the current packet, but it isn't exactly
              // the packet end. Just fragment, fix the list, and return.
              Ptr<Packet> p = current->CreateFragment (0, numBytes);
              current->RemoveAtStart (numBytes);
              list.insert (it, p);
              return p;
            }
        }
      else
        {
          // The end isn't inside current packet, but there is an exception for
          // the merge and recurse strategy...
          if (++it == list.end ())
            {
              // ...current is the last packet we sent. We have not more data;
              // Go for this one.
              NS_LOG_WARN ("Cannot reach the end, but this case is covered "
                           "with conditional statements inside CopyFromSequence."
                           "Something has gone wrong, report a bug");
              return outPacket;
            }

          // The current packet does not contain the requested end. Merge current
          // with the packet that follows, and recurse
          Ptr<Packet> next = (*it); // Please remember we have incremented it
                                    // in the previous if
          current->AddAtEnd (next);
          list.erase (it);
          return GetPacketFromList (list, listStartFrom, numBytes, seq);
        }
    }

  NS_FATAL_ERROR ("This point is not reachable");
}


void
TcpTxBuffer::DiscardUpTo (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);

  // Cases do not need to scan the buffer
  if (m_firstByteSeq >= seq)
    {
      NS_LOG_DEBUG ("Seq " << seq << " already discarded.");
      return;
    }

  // Scan the buffer and discard packets
  uint32_t offset = seq - m_firstByteSeq.Get ();  // Number of bytes to remove
  uint32_t pktSize;
  PacketList::iterator i = m_sentList.begin ();
  while (i != m_sentList.end ())
    {
      if (offset >= (*i)->GetSize ())
        { // This packet is behind the seqnum. Remove this packet from the buffer
          pktSize = (*i)->GetSize ();
          m_size -= pktSize;
          m_sentSize -= pktSize;
          offset -= pktSize;
          m_firstByteSeq += pktSize;
          i = m_sentList.erase (i);
          NS_LOG_INFO ("While removing up to " << seq <<
                       ".Removed one packet of size " << pktSize <<
                       " starting from " << m_firstByteSeq - pktSize <<
                       ". Remaining data " << m_size);
        }
      else if (offset > 0)
        { // Part of the packet is behind the seqnum. Fragment
          pktSize = (*i)->GetSize () - offset;
          *i = (*i)->CreateFragment (offset, pktSize);
          m_size -= offset;
          m_sentSize -= offset;
          m_firstByteSeq += offset;
          NS_LOG_INFO ("Fragmented one packet by size " << offset <<
                       ", new size=" << pktSize);
          break;
        }
      else
        {
          // offset is 0, so we have discarded data up to seq.
          break;
        }
    }
  // Catching the case of ACKing a FIN
  if (m_size == 0)
    {
      m_firstByteSeq = seq;
    }

  NS_LOG_DEBUG ("Discarded up to " << seq);
  NS_LOG_LOGIC ("Buffer status after discarding data " << *this);
}

std::ostream &
operator<< (std::ostream & os, TcpTxBuffer const & tcpTxBuf)
{
  TcpTxBuffer::PacketList::const_iterator it;
  std::stringstream ss;
  SequenceNumber32 beginOfCurrentPacket = tcpTxBuf.m_firstByteSeq;
  uint32_t sentSize = 0, appSize = 0;

  for (it = tcpTxBuf.m_sentList.begin ();
       it != tcpTxBuf.m_sentList.end (); ++it)
    {
      ss << "[" << beginOfCurrentPacket << ";"
         << beginOfCurrentPacket + (*it)->GetSize () << "|" << (*it)->GetSize () << "|]";
      sentSize += (*it)->GetSize ();
      beginOfCurrentPacket += (*it)->GetSize ();
    }

  for (it = tcpTxBuf.m_appList.begin ();
       it != tcpTxBuf.m_appList.end (); ++it)
    {
      appSize += (*it)->GetSize ();
    }

  os << "Sent list: " << ss.str () << ", size = " << tcpTxBuf.m_sentList.size () <<
    " Total size: " << tcpTxBuf.m_size <<
    " m_firstByteSeq = " << tcpTxBuf.m_firstByteSeq <<
    " m_sentSize = " << tcpTxBuf.m_sentSize;

  NS_ASSERT (sentSize == tcpTxBuf.m_sentSize);
  NS_ASSERT (tcpTxBuf.m_size - tcpTxBuf.m_sentSize == appSize);
  return os;
}

} // namepsace ns3
