/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 * Copyright (c) 2010 Adrian Sai-wah Tam
 * Copyright (c) 2016 Natale Patriciello
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
 * Original Author: Adrian Sai-wah Tam <adrian.sw.tam@gmail.com>
 */

#define NS_LOG_APPEND_CONTEXT \
  if (m_node) { std::clog << " [node " << m_node->GetId () << "] "; }

#include "ns3-tcp-socket-impl.h"
#include "tcp-congestion-ops.h"
#include "tcp-socket-base.h"
#include "rtt-estimator.h"
#include "tcp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include "tcp-rx-buffer.h"
#include "tcp-tx-buffer.h"
#include "tcp-option-ts.h"
#include "tcp-option-sack.h"
#include "tcp-option-sack-permitted.h"
#include "tcp-option-winscale.h"
#include "ipv4.h"
#include "ipv6.h"
#include "ipv4-routing-protocol.h"
#include "ipv6-routing-protocol.h"
#include "ipv6-l3-protocol.h"
#include "ipv6-route.h"

#include "ns3/log.h"
#include "ns3/abort.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("Ns3TcpSocketImpl");

NS_OBJECT_ENSURE_REGISTERED (Ns3TcpSocketImpl);

TypeId
Ns3TcpSocketImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::Ns3TcpSocketImpl")
    .SetParent<TcpImplementation> ()
    .SetGroupName ("Internet")
    .AddConstructor<Ns3TcpSocketImpl> ()
  ;
  return tid;
}

Ns3TcpSocketImpl::Ns3TcpSocketImpl ()
{

}

Ns3TcpSocketImpl::~Ns3TcpSocketImpl ()
{

}

TypeId
Ns3TcpSocketImpl::GetInstanceTypeId () const
{
  return Ns3TcpSocketImpl::GetTypeId ();
}

enum Socket::SocketErrno
Ns3TcpSocketImpl::GetErrno (void) const
{
  return m_errno;
}

enum Socket::SocketType
Ns3TcpSocketImpl::GetSocketType (void) const
{
  return Socket::NS3_SOCK_STREAM;
}

Ptr<Node>
Ns3TcpSocketImpl::GetNode (void) const
{
  return m_node;
}

void
Ns3TcpSocketImpl::SetNode (Ptr<Node> node)
{
  m_node = node;
}

void
Ns3TcpSocketImpl::SetL4Protocol (Ptr<TcpL4Protocol> tcp)
{
  m_tcp = tcp;
}

void
Ns3TcpSocketImpl::SetRtt (Ptr<RttEstimator> rtt)
{
  m_rtt = rtt;
}

void
Ns3TcpSocketImpl::SetTcpSocket(Ptr<TcpSocketBase> socket)
{
  m_socket = socket;
}

void
Ns3TcpSocketImpl::SetTracedValues(const TcpTracedValues &traced)
{
  m_tracedValues = traced;
}

double
Ns3TcpSocketImpl::GetMsl (void) const
{
  return m_msl;
}
void
Ns3TcpSocketImpl::SetMsl (double msl)
{
  m_msl = msl;
}
void
Ns3TcpSocketImpl::SetMaxWinSize (uint16_t maxWinSize)
{
  m_maxWinSize = maxWinSize;
}
uint16_t
Ns3TcpSocketImpl::GetMaxWinSize () const
{
  return m_maxWinSize;
}
bool
Ns3TcpSocketImpl::GetWinScaleEnabled () const
{
  return m_winScalingEnabled;
}
void
Ns3TcpSocketImpl::SetWinScaleEnabled (bool enabled)
{
  m_winScalingEnabled = enabled;
}
bool
Ns3TcpSocketImpl::GetSACKEnabled () const
{
  return m_sackEnabled;
}
void
Ns3TcpSocketImpl::SetSACKEnabled (bool enabled)
{
  m_sackEnabled = enabled;
}

bool
Ns3TcpSocketImpl::GetTimestampEnabled () const
{
  return m_timestampEnabled;
}
void
Ns3TcpSocketImpl::SetTimestampEnabled (bool enabled)
{
  m_timestampEnabled = enabled;
}
void
Ns3TcpSocketImpl::SetRxThresh (uint32_t rxThresh)
{
  m_retxThresh = rxThresh;
}
uint32_t
Ns3TcpSocketImpl::GetRxThresh () const
{
  return m_retxThresh;
}
bool
Ns3TcpSocketImpl::GetLimitedTx () const
{
  return m_limitedTx;
}
void
Ns3TcpSocketImpl::SetLimitedTx (bool enabled)
{
  m_limitedTx = enabled;
}

void
Ns3TcpSocketImpl::SetMinRto (Time minRto)
{
  m_minRto = minRto;
}
Time
Ns3TcpSocketImpl::GetMinRto (void) const
{
  return m_minRto;
}
void
Ns3TcpSocketImpl::SetClockGranularity (Time clockGranularity)
{
  m_clockGranularity = clockGranularity;
}
Time
Ns3TcpSocketImpl::GetClockGranularity (void) const
{
  return m_clockGranularity;
}
Ptr<TcpTxBuffer>
Ns3TcpSocketImpl::GetTxBuffer (void) const
{
  return m_txBuffer;
}
Ptr<TcpRxBuffer>
Ns3TcpSocketImpl::GetRxBuffer (void) const
{
  return m_rxBuffer;
}
uint16_t
Ns3TcpSocketImpl::GetLocalPort() const
{
  if (m_endPoint != 0)
    {
      return m_endPoint->GetLocalPort ();
    }
  else if (m_endPoint6 != 0)
    {
      return m_endPoint6->GetLocalPort ();
    }
  else
    {
      return 0;
    }
}
void
Ns3TcpSocketImpl::SetTcpNoDelay (bool noDelay)
{
  NS_LOG_FUNCTION (this << noDelay);
  m_noDelay = noDelay;
}
bool
Ns3TcpSocketImpl::SetAllowBroadcast (bool allowBroadcast)
{
  // Broadcast is not implemented. Return true only if allowBroadcast==false
  return (!allowBroadcast);
}
uint32_t
Ns3TcpSocketImpl::GetInitialSSThresh (void) const
{
  return m_tcb->m_initialSsThresh;
}
Time
Ns3TcpSocketImpl::GetConnTimeout (void) const
{
  return m_cnTimeout;
}
void
Ns3TcpSocketImpl::SetSndBufSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_txBuffer->SetMaxBufferSize (size);
}
uint32_t
Ns3TcpSocketImpl::GetSndBufSize (void) const
{
  return m_txBuffer->MaxBufferSize ();
}
uint32_t
Ns3TcpSocketImpl::GetRcvBufSize (void) const
{
  return m_rxBuffer->MaxBufferSize ();
}
void
Ns3TcpSocketImpl::SetSegSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_tcb->m_segmentSize = size;

  NS_ABORT_MSG_UNLESS (m_state == TcpSocket::CLOSED, "Cannot change segment size dynamically.");
}
uint32_t
Ns3TcpSocketImpl::GetSegSize (void) const
{
  return m_tcb->m_segmentSize;
}
void
Ns3TcpSocketImpl::SetConnTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_cnTimeout = timeout;
}
void
Ns3TcpSocketImpl::SetSynRetries (uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_synRetries = count;
}
uint32_t
Ns3TcpSocketImpl::GetSynRetries (void) const
{
  return m_synRetries;
}
void
Ns3TcpSocketImpl::SetDataRetries (uint32_t retries)
{
  NS_LOG_FUNCTION (this << retries);
  m_dataRetries = retries;
}
uint32_t
Ns3TcpSocketImpl::GetDataRetries (void) const
{
  NS_LOG_FUNCTION (this);
  return m_dataRetries;
}
void
Ns3TcpSocketImpl::SetDelAckTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_delAckTimeout = timeout;
}
Time
Ns3TcpSocketImpl::GetDelAckTimeout (void) const
{
  return m_delAckTimeout;
}
void
Ns3TcpSocketImpl::SetDelAckMaxCount (uint32_t count)
{
  NS_LOG_FUNCTION (this << count);
  m_delAckMaxCount = count;
}
uint32_t
Ns3TcpSocketImpl::GetDelAckMaxCount (void) const
{
  return m_delAckMaxCount;
}
bool
Ns3TcpSocketImpl::GetTcpNoDelay (void) const
{
  return m_noDelay;
}
void
Ns3TcpSocketImpl::SetPersistTimeout (Time timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_persistTimeout = timeout;
}
Time
Ns3TcpSocketImpl::GetPersistTimeout (void) const
{
  return m_persistTimeout;
}
bool
Ns3TcpSocketImpl::GetAllowBroadcast (void) const
{
  return false;
}
void
Ns3TcpSocketImpl::SetInitialSSThresh (uint32_t threshold)
{
  NS_ABORT_MSG_UNLESS ((m_state == TcpSocket::CLOSED) || threshold == m_tcb->m_initialSsThresh,
                       "Ns3TcpSocketImpl::SetSSThresh() cannot change initial ssThresh after connection started.");

  m_tcb->m_initialSsThresh = threshold;
}
void
Ns3TcpSocketImpl::SetInitialCwnd (uint32_t cwnd)
{
  NS_ABORT_MSG_UNLESS ((m_state == TcpSocket::CLOSED) || cwnd == m_tcb->m_initialCWnd,
                       "Ns3TcpSocketImpl::SetInitialCwnd() cannot change initial cwnd after connection started.");

  m_tcb->m_initialCWnd = cwnd;
}
uint32_t
Ns3TcpSocketImpl::GetInitialCwnd (void) const
{
  return m_tcb->m_initialCWnd;
}
/* Inherit from Socket class: Get the max number of bytes an app can send */
uint32_t
Ns3TcpSocketImpl::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_txBuffer->Available ();
}

/* Inherit from Socket class: Get the max number of bytes an app can read */
uint32_t
Ns3TcpSocketImpl::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION (this);
  return m_rxBuffer->Available ();
}

/* Inherit from Socket class: Return local address:port */
int
Ns3TcpSocketImpl::GetSockName (Address &address) const
{
  NS_LOG_FUNCTION (this);
  if (m_endPoint != 0)
    {
      address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
    }
  else if (m_endPoint6 != 0)
    {
      address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
    }
  else
    { // It is possible to call this method on a socket without a name
      // in which case, behavior is unspecified
      // Should this return an InetSocketAddress or an Inet6SocketAddress?
      address = InetSocketAddress (Ipv4Address::GetZero (), 0);
    }
  return 0;
}
int
Ns3TcpSocketImpl::GetPeerName (Address &address) const
{
  NS_LOG_FUNCTION (this << address);

  if (!m_endPoint && !m_endPoint6)
    {
      m_errno = Socket::ERROR_NOTCONN;
      return -1;
    }

  if (m_endPoint)
    {
      address = InetSocketAddress (m_endPoint->GetPeerAddress (),
                                   m_endPoint->GetPeerPort ());
    }
  else if (m_endPoint6)
    {
      address = Inet6SocketAddress (m_endPoint6->GetPeerAddress (),
                                    m_endPoint6->GetPeerPort ());
    }
  else
    {
      NS_ASSERT (false);
    }

  return 0;
}
void
Ns3TcpSocketImpl::SetCongestionControlAlgorithm (Ptr<TcpCongestionOps> algo)
{
  NS_LOG_FUNCTION (this << algo);
  m_congestionControl = algo;
}
void
Ns3TcpSocketImpl::SetRcvBufSize (uint32_t size)
{
  NS_LOG_FUNCTION (this << size);
  uint32_t oldSize = GetRcvBufSize ();

  m_rxBuffer->SetMaxBufferSize (size);

  /* The size has (manually) increased. Actively inform the other end to prevent
   * stale zero-window states.
   */
  if (oldSize < size && m_connected)
    {
      //SendEmptyPacket (TcpHeader::ACK);
    }
}


/* Inherit from Socket class: Bind this socket to the specified NetDevice */
void
Ns3TcpSocketImpl::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice);

  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  m_endPoint->BindToNetDevice (netdevice);

  if (m_endPoint6 == 0)
    {
      if (Bind6 () == -1)
        {
          NS_ASSERT (m_endPoint6 == 0);
          return;
        }
      NS_ASSERT (m_endPoint6 != 0);
    }
  m_endPoint6->BindToNetDevice (netdevice);

  return;
}
/* Inherit from Socket class: Bind socket (with specific address) to an end-point in TcpL4Protocol */
int
Ns3TcpSocketImpl::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);
  if (InetSocketAddress::IsMatchingType (address))
    {
      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      Ipv4Address ipv4 = transport.GetIpv4 ();
      uint16_t port = transport.GetPort ();

      if (ipv4 == Ipv4Address::GetAny () && port == 0)
        {
          m_endPoint = m_tcp->Allocate ();
        }
      else if (ipv4 == Ipv4Address::GetAny () && port != 0)
        {
          m_endPoint = m_tcp->Allocate (port);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port == 0)
        {
          m_endPoint = m_tcp->Allocate (ipv4);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port != 0)
        {
          m_endPoint = m_tcp->Allocate (ipv4, port);
        }
      if (0 == m_endPoint)
        {
          m_errno = port ? Socket::ERROR_ADDRINUSE : Socket::ERROR_ADDRNOTAVAIL;
          return -1;
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (address))
    {
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address ipv6 = transport.GetIpv6 ();
      uint16_t port = transport.GetPort ();
      if (ipv6 == Ipv6Address::GetAny () && port == 0)
        {
          m_endPoint6 = m_tcp->Allocate6 ();
        }
      else if (ipv6 == Ipv6Address::GetAny () && port != 0)
        {
          m_endPoint6 = m_tcp->Allocate6 (port);
        }
      else if (ipv6 != Ipv6Address::GetAny () && port == 0)
        {
          m_endPoint6 = m_tcp->Allocate6 (ipv6);
        }
      else if (ipv6 != Ipv6Address::GetAny () && port != 0)
        {
          m_endPoint6 = m_tcp->Allocate6 (ipv6, port);
        }
      if (0 == m_endPoint6)
        {
          m_errno = port ? Socket::ERROR_ADDRINUSE : Socket::ERROR_ADDRNOTAVAIL;
          return -1;
        }
    }
  else
    {
      m_errno = Socket::ERROR_INVAL;
      return -1;
    }

  m_tcp->AddSocket (m_socket);

  NS_LOG_LOGIC ("Ns3TcpSocketImpl " << this << " got an endpoint: " << m_endPoint);

  return SetupCallback ();
}
int
Ns3TcpSocketImpl::Bind6 (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint6 = m_tcp->Allocate6 ();
  if (0 == m_endPoint6)
    {
      m_errno = Socket::ERROR_ADDRNOTAVAIL;
      return -1;
    }

  m_tcp->AddSocket (m_socket);

  return SetupCallback ();
}
int
Ns3TcpSocketImpl::Bind (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint = m_tcp->Allocate ();
  if (0 == m_endPoint)
    {
      m_errno = Socket::ERROR_ADDRNOTAVAIL;
      return -1;
    }

  m_tcp->AddSocket (m_socket);

  return SetupCallback ();
}
int
Ns3TcpSocketImpl::SetupCallback (void)
{
  NS_LOG_FUNCTION (this);

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      return -1;
    }
  if (m_endPoint != 0)
    {
      m_endPoint->SetRxCallback (MakeCallback (&TcpSocketBase::ForwardUp, m_socket));
      m_endPoint->SetIcmpCallback (MakeCallback (&TcpSocketBase::ForwardIcmp, m_socket));
      m_endPoint->SetDestroyCallback (MakeCallback (&TcpSocketBase::Destroy, m_socket));
    }
  if (m_endPoint6 != 0)
    {
      m_endPoint6->SetRxCallback (MakeCallback (&TcpSocketBase::ForwardUp6, m_socket));
      m_endPoint6->SetIcmpCallback (MakeCallback (&TcpSocketBase::ForwardIcmp6, m_socket));
      m_endPoint6->SetDestroyCallback (MakeCallback (&TcpSocketBase::Destroy6, m_socket));
    }

  return 0;
}

void
Ns3TcpSocketImpl::Destroy (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint = 0;
  if (m_tcp != 0)
    {
      m_tcp->RemoveSocket (m_socket);
    }
  CancelAllTimers ();
}

void
Ns3TcpSocketImpl::Destroy6 (void)
{
  NS_LOG_FUNCTION (this);
  m_endPoint6 = 0;
  if (m_tcp != 0)
    {
      m_tcp->RemoveSocket (m_socket);
    }
  CancelAllTimers ();
}

void
Ns3TcpSocketImpl::CancelAllTimers ()
{
  m_retxEvent.Cancel ();
  m_persistEvent.Cancel ();
  m_delAckEvent.Cancel ();
  m_lastAckEvent.Cancel ();
  m_timewaitEvent.Cancel ();
  m_sendPendingDataEvent.Cancel ();
}

Ptr<Packet>
Ns3TcpSocketImpl::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this);
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in Recv()");
  if (m_rxBuffer->Size () == 0 && m_state == TcpSocket::CLOSE_WAIT)
    {
      return Create<Packet> (); // Send EOF on connection close
    }
  Ptr<Packet> outPacket = m_rxBuffer->Extract (maxSize);
  return outPacket;
}

Ptr<Packet>
Ns3TcpSocketImpl::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  Ptr<Packet> packet = Recv (maxSize, flags);
  // Null packet means no data to read, and an empty packet indicates EOF
  if (packet != 0 && packet->GetSize () != 0)
    {
      if (m_endPoint != 0)
        {
          fromAddress = InetSocketAddress (m_endPoint->GetPeerAddress (), m_endPoint->GetPeerPort ());
        }
      else if (m_endPoint6 != 0)
        {
          fromAddress = Inet6SocketAddress (m_endPoint6->GetPeerAddress (), m_endPoint6->GetPeerPort ());
        }
      else
        {
          fromAddress = InetSocketAddress (Ipv4Address::GetZero (), 0);
        }
    }
  return packet;
}

int
Ns3TcpSocketImpl::ShutdownRecv (void)
{
  NS_LOG_FUNCTION (this);
  m_shutdownRecv = true;
  return 0;
}

int
Ns3TcpSocketImpl::ShutdownSend (void)
{
  NS_LOG_FUNCTION (this);

  //this prevents data from being added to the buffer
  m_shutdownSend = true;
  m_closeOnEmpty = true;
  //if buffer is already empty, send a fin now
  //otherwise fin will go when buffer empties.
  if (m_txBuffer->Size () == 0)
    {
      if (m_state == TcpSocket::ESTABLISHED || m_state == TcpSocket::CLOSE_WAIT)
        {
          NS_LOG_INFO ("Emtpy tx buffer, send fin");
          SendEmptyPacket (TcpHeader::FIN);

          if (m_state == TcpSocket::ESTABLISHED)
            { // On active close: I am the first one to send FIN
              NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
              m_state = TcpSocket::FIN_WAIT_1;
            }
          else
            { // On passive close: Peer sent me FIN already
              NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
              m_state = TcpSocket::LAST_ACK;
            }
        }
    }

  return 0;
}

void
Ns3TcpSocketImpl::SendEmptyPacket (uint8_t flags)
{
  NS_LOG_FUNCTION (this << (uint32_t)flags);
  Ptr<Packet> p = Create<Packet> ();
  TcpHeader header;
  SequenceNumber32 s = m_tracedValues.GetNextTxSequence ();

  /*
   * Add tags for each socket option.
   * Note that currently the socket adds both IPv4 tag and IPv6 tag
   * if both options are set. Once the packet got to layer three, only
   * the corresponding tags will be read.
   */
  if (m_socket->GetIpTos ())
    {
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (m_socket->GetIpTos ());
      p->AddPacketTag (ipTosTag);
    }

  if (m_socket->IsManualIpv6Tclass ())
    {
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (m_socket->GetIpv6Tclass ());
      p->AddPacketTag (ipTclassTag);
    }

  if (m_socket->IsManualIpTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (m_socket->GetIpTtl ());
      p->AddPacketTag (ipTtlTag);
    }

  if (m_socket->IsManualIpv6HopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (m_socket->GetIpv6HopLimit ());
      p->AddPacketTag (ipHopLimitTag);
    }

  uint8_t priority = m_socket->GetPriority ();
  if (priority)
    {
      SocketPriorityTag priorityTag;
      priorityTag.SetPriority (priority);
      p->ReplacePacketTag (priorityTag);
    }

  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_WARN ("Failed to send empty packet due to null endpoint");
      return;
    }
  if (flags & TcpHeader::FIN)
    {
      flags |= TcpHeader::ACK;
    }
  else if (m_state == TcpSocket::FIN_WAIT_1
           || m_state == TcpSocket::LAST_ACK
           || m_state == TcpSocket::CLOSING)
    {
      ++s;
    }

  header.SetFlags (flags);
  header.SetSequenceNumber (s);
  header.SetAckNumber (m_rxBuffer->NextRxSequence ());
  if (m_endPoint != 0)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  AddOptions (header);

  // RFC 6298, clause 2.4
  m_tracedValues.SetRto(Max (m_rtt->GetEstimate () +
                             Max (m_clockGranularity, m_rtt->GetVariation () * 4), m_minRto));

  uint16_t windowSize = AdvertisedWindowSize ();
  bool hasSyn = flags & TcpHeader::SYN;
  bool hasFin = flags & TcpHeader::FIN;
  bool isAck = flags == TcpHeader::ACK;
  if (hasSyn)
    {
      if (m_winScalingEnabled)
        { // The window scaling option is set only on SYN packets
          AddOptionWScale (header);
        }

      if (m_sackEnabled)
        {
          AddOptionSACKPermitted (header);
        }

      if (m_synCount == 0)
        { // No more connection retries, give up
          NS_LOG_LOGIC ("Connection failed.");
          m_rtt->Reset (); //According to recommendation -> RFC 6298
          CloseAndNotify ();
          return;
        }
      else
        { // Exponential backoff of connection time out
          int backoffCount = 0x1 << (m_synRetries - m_synCount);
          m_tracedValues.SetRto(m_cnTimeout * backoffCount);
          m_synCount--;
        }

      if (m_synRetries - 1 == m_synCount)
        {
          UpdateRttHistory (s, 0, false);
        }
      else
        { // This is SYN retransmission
          UpdateRttHistory (s, 0, true);
        }

      windowSize = AdvertisedWindowSize (false);
    }
  header.SetWindowSize (windowSize);

  if (flags & TcpHeader::ACK)
    { // If sending an ACK, cancel the delay ACK as well
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
      if (m_highTxAck < header.GetAckNumber ())
        {
          m_highTxAck = header.GetAckNumber ();
        }
      else
        {
          if (m_sackEnabled)
            {
              AddOptionSACK (header);
            }
        }
    }

  m_tracedValues.TxTrace (p, header, m_socket);

  if (m_endPoint != 0)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_socket->GetBoundNetDevice ());
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_socket->GetBoundNetDevice ());
    }


  if (m_retxEvent.IsExpired () && (hasSyn || hasFin) && !isAck )
    { // Retransmit SYN / SYN+ACK / FIN / FIN+ACK to guard against lost
      NS_LOG_LOGIC ("Schedule retransmission timeout at time "
                    << Simulator::Now ().GetSeconds () << " to expire at time "
                    << (Simulator::Now () + m_tracedValues.GetRto ()).GetSeconds ());
      m_retxEvent = Simulator::Schedule (m_tracedValues.GetRto (),
                                         &Ns3TcpSocketImpl::SendEmptyPacket,
                                         this, flags);
    }
}
void
Ns3TcpSocketImpl::AddOptions (TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  if (m_timestampEnabled)
    {
      AddOptionTimestamp (header);
    }
}
void
Ns3TcpSocketImpl::ProcessOptionWScale (const Ptr<const TcpOption> option)
{
  NS_LOG_FUNCTION (this << option);

  Ptr<const TcpOptionWinScale> ws = DynamicCast<const TcpOptionWinScale> (option);

  // In naming, we do the contrary of RFC 1323. The received scaling factor
  // is Rcv.Wind.Scale (and not Snd.Wind.Scale)
  m_sndWindShift = ws->GetScale ();

  if (m_sndWindShift > 14)
    {
      NS_LOG_WARN ("Possible error; m_sndWindShift exceeds 14: " << m_sndWindShift);
      m_sndWindShift = 14;
    }

  NS_LOG_INFO (m_node->GetId () << " Received a scale factor of " <<
               static_cast<int> (m_sndWindShift));
}

uint8_t
Ns3TcpSocketImpl::CalculateWScale () const
{
  NS_LOG_FUNCTION (this);
  uint32_t maxSpace = m_rxBuffer->MaxBufferSize ();
  uint8_t scale = 0;

  while (maxSpace > m_maxWinSize)
    {
      maxSpace = maxSpace >> 1;
      ++scale;
    }

  if (scale > 14)
    {
      NS_LOG_WARN ("Possible error; scale exceeds 14: " << scale);
      scale = 14;
    }

  NS_LOG_INFO ("Node " << m_node->GetId () << " calculated wscale factor of " <<
               static_cast<int> (scale) << " for buffer size " << m_rxBuffer->MaxBufferSize ());
  return scale;
}

void
Ns3TcpSocketImpl::AddOptionWScale (TcpHeader &header)
{
  NS_LOG_FUNCTION (this << header);
  NS_ASSERT (header.GetFlags () & TcpHeader::SYN);

  Ptr<TcpOptionWinScale> option = CreateObject<TcpOptionWinScale> ();

  // In naming, we do the contrary of RFC 1323. The sended scaling factor
  // is Snd.Wind.Scale (and not Rcv.Wind.Scale)

  m_rcvWindShift = CalculateWScale ();
  option->SetScale (m_rcvWindShift);

  header.AppendOption (option);

  NS_LOG_INFO (m_node->GetId () << " Send a scaling factor of " <<
               static_cast<int> (m_rcvWindShift));
}

bool
Ns3TcpSocketImpl::ProcessOptionSACK (const Ptr<const TcpOption> option)
{
  NS_LOG_FUNCTION (this << option);

  Ptr<const TcpOptionSack> s = DynamicCast<const TcpOptionSack> (option);
  TcpOptionSack::SackList list = s->GetSackList ();
  return m_txBuffer->Update (list);
}

void
Ns3TcpSocketImpl::ProcessOptionSACKPermitted (const Ptr<const TcpOption> option)
{
  NS_LOG_FUNCTION (this << option);

  Ptr<const TcpOptionSackPermitted> s = DynamicCast<const TcpOptionSackPermitted> (option);

  NS_ASSERT (m_sackEnabled == true);
  NS_LOG_INFO (m_node->GetId () << " Received a SACK_PERMITTED option " << s);
}

void
Ns3TcpSocketImpl::AddOptionSACKPermitted (TcpHeader &header)
{
  NS_LOG_FUNCTION (this << header);
  NS_ASSERT (header.GetFlags () & TcpHeader::SYN);

  Ptr<TcpOptionSackPermitted> option = CreateObject<TcpOptionSackPermitted> ();
  header.AppendOption (option);
  NS_LOG_INFO (m_node->GetId () << " Add option SACK-PERMITTED");
}

void
Ns3TcpSocketImpl::AddOptionSACK (TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  // Calculate the number of SACK blocks allowed in this packet
  uint8_t optionLenAvail = header.GetMaxOptionLength () - header.GetOptionLength ();
  uint8_t allowedSackBlocks = (optionLenAvail - 2) / 8;

  TcpOptionSack::SackList sackList = m_rxBuffer->GetSackList ();
  if (allowedSackBlocks == 0 || sackList.empty ())
    {
      NS_LOG_LOGIC ("No space available or sack list empty, not adding sack blocks");
      return;
    }

  // Append the allowed number of SACK blocks
  Ptr<TcpOptionSack> option = CreateObject<TcpOptionSack> ();
  TcpOptionSack::SackList::iterator i;
  for (i = sackList.begin (); allowedSackBlocks > 0 && i != sackList.end (); ++i)
    {
      NS_LOG_LOGIC ("Left edge of the block: " << (*i).first << "Right edge of the block: " << (*i).second);
      option->AddSackBlock (*i);
      allowedSackBlocks--;
    }

  header.AppendOption (option);
  NS_LOG_INFO (m_node->GetId () << " Add option SACK");
}

void
Ns3TcpSocketImpl::ProcessOptionTimestamp (const Ptr<const TcpOption> option,
                                       const SequenceNumber32 &seq)
{
  NS_LOG_FUNCTION (this << option);

  Ptr<const TcpOptionTS> ts = DynamicCast<const TcpOptionTS> (option);

  if (seq == m_rxBuffer->NextRxSequence () && seq <= m_highTxAck)
    {
      m_timestampToEcho = ts->GetTimestamp ();
    }

  NS_LOG_INFO (m_node->GetId () << " Got timestamp=" <<
               m_timestampToEcho << " and Echo="     << ts->GetEcho ());
}

void
Ns3TcpSocketImpl::AddOptionTimestamp (TcpHeader& header)
{
  NS_LOG_FUNCTION (this << header);

  Ptr<TcpOptionTS> option = CreateObject<TcpOptionTS> ();

  option->SetTimestamp (TcpOptionTS::NowToTsValue ());
  option->SetEcho (m_timestampToEcho);

  header.AppendOption (option);
  NS_LOG_INFO (m_node->GetId () << " Add option TS, ts=" <<
               option->GetTimestamp () << " echo=" << m_timestampToEcho);
}

uint16_t
Ns3TcpSocketImpl::AdvertisedWindowSize (bool scale) const
{
  NS_LOG_FUNCTION (this << scale);
  uint32_t w = m_rxBuffer->MaxBufferSize ();

  if (scale)
    {
      w >>= m_rcvWindShift;
    }
  if (w > m_maxWinSize)
    {
      w = m_maxWinSize;
      NS_LOG_WARN ("Adv window size truncated to " << m_maxWinSize <<
                   "; possibly to avoid overflow of the 16-bit integer");
    }
  NS_LOG_INFO ("Returning AdvertisedWindowSize of " << static_cast<uint16_t> (w));
  return static_cast<uint16_t> (w);
}

/* Peacefully close the socket by notifying the upper layer and deallocate end point */
void
Ns3TcpSocketImpl::CloseAndNotify (void)
{
  NS_LOG_FUNCTION (this);

  if (!m_closeNotified)
    {
      m_socket->NotifyNormalClose ();
      m_closeNotified = true;
    }

  NS_LOG_DEBUG (TcpSocket::TcpStateName[m_state] << " -> CLOSED");
  m_state = TcpSocket::CLOSED;
  DeallocateEndPoint ();
}
void
Ns3TcpSocketImpl::UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                                    bool isRetransmission)
{
  NS_LOG_FUNCTION (this);

  // update the history of sequence numbers used to calculate the RTT
  if (isRetransmission == false)
    { // This is the next expected one, just log at end
      m_history.push_back (RttHistory (seq, sz, Simulator::Now ()));
    }
  else
    { // This is a retransmit, find in list and mark as re-tx
      for (RttHistory_t::iterator i = m_history.begin (); i != m_history.end (); ++i)
        {
          if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
            { // Found it
              i->retx = true;
              i->count = ((seq + SequenceNumber32 (sz)) - i->seq); // And update count in hist
              break;
            }
        }
    }
}
/* Deallocate the end point and cancel all the timers */
void
Ns3TcpSocketImpl::DeallocateEndPoint (void)
{
  if (m_endPoint != 0)
    {
      CancelAllTimers ();
      m_endPoint->SetDestroyCallback (MakeNullCallback<void> ());
      m_tcp->DeAllocate (m_endPoint);
      m_endPoint = 0;
      m_tcp->RemoveSocket (m_socket);
    }
  else if (m_endPoint6 != 0)
    {
      CancelAllTimers ();
      m_endPoint6->SetDestroyCallback (MakeNullCallback<void> ());
      m_tcp->DeAllocate (m_endPoint6);
      m_endPoint6 = 0;
      m_tcp->RemoveSocket (m_socket);
    }
}
/* Inherit from Socket class: Initiate connection to a remote address:port */
int
Ns3TcpSocketImpl::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);

  // If haven't do so, Bind() this socket first
  if (InetSocketAddress::IsMatchingType (address) && m_endPoint6 == 0)
    {
      if (m_endPoint == 0)
        {
          if (Bind () == -1)
            {
              NS_ASSERT (m_endPoint == 0);
              return -1; // Bind() failed
            }
          NS_ASSERT (m_endPoint != 0);
        }
      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      m_endPoint->SetPeer (transport.GetIpv4 (), transport.GetPort ());
      m_socket->SetIpTos (transport.GetTos ());
      m_endPoint6 = 0;

      // Get the appropriate local address and port number from the routing protocol and set up endpoint
      if (SetupEndpoint () != 0)
        {
          NS_LOG_ERROR ("Route to destination does not exist ?!");
          return -1;
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (address)  && m_endPoint == 0)
    {
      // If we are operating on a v4-mapped address, translate the address to
      // a v4 address and re-call this function
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address v6Addr = transport.GetIpv6 ();
      if (v6Addr.IsIpv4MappedAddress () == true)
        {
          Ipv4Address v4Addr = v6Addr.GetIpv4MappedAddress ();
          return Connect (InetSocketAddress (v4Addr, transport.GetPort ()));
        }

      if (m_endPoint6 == 0)
        {
          if (Bind6 () == -1)
            {
              NS_ASSERT (m_endPoint6 == 0);
              return -1; // Bind() failed
            }
          NS_ASSERT (m_endPoint6 != 0);
        }
      m_endPoint6->SetPeer (v6Addr, transport.GetPort ());
      m_endPoint = 0;

      // Get the appropriate local address and port number from the routing protocol and set up endpoint
      if (SetupEndpoint6 () != 0)
        { // Route to destination does not exist
          return -1;
        }
    }
  else
    {
      m_errno = TcpSocket::ERROR_INVAL;
      return -1;
    }

  // Re-initialize parameters in case this socket is being reused after CLOSE
  m_rtt->Reset ();
  m_synCount = m_synRetries;
  m_dataRetrCount = m_dataRetries;

  // DoConnect() will do state-checking and send a SYN packet
  return DoConnect ();
}
/* Configure the endpoint to a local address. Called by Connect() if Bind() didn't specify one. */
int
Ns3TcpSocketImpl::SetupEndpoint ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();
  NS_ASSERT (ipv4 != 0);
  if (ipv4->GetRoutingProtocol () == 0)
    {
      NS_FATAL_ERROR ("No Ipv4RoutingProtocol in the node");
    }
  // Create a dummy packet, then ask the routing function for the best output
  // interface's address
  Ipv4Header header;
  header.SetDestination (m_endPoint->GetPeerAddress ());
  Socket::SocketErrno errno_;
  Ptr<Ipv4Route> route;
  Ptr<NetDevice> oif = m_socket->GetBoundNetDevice ();
  route = ipv4->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
  if (route == 0)
    {
      NS_LOG_LOGIC ("Route to " << m_endPoint->GetPeerAddress () << " does not exist");
      NS_LOG_ERROR (errno_);
      m_errno = errno_;
      return -1;
    }
  NS_LOG_LOGIC ("Route exists");
  m_endPoint->SetLocalAddress (route->GetSource ());
  return 0;
}

int
Ns3TcpSocketImpl::SetupEndpoint6 ()
{
  NS_LOG_FUNCTION (this);
  Ptr<Ipv6L3Protocol> ipv6 = m_node->GetObject<Ipv6L3Protocol> ();
  NS_ASSERT (ipv6 != 0);
  if (ipv6->GetRoutingProtocol () == 0)
    {
      NS_FATAL_ERROR ("No Ipv6RoutingProtocol in the node");
    }
  // Create a dummy packet, then ask the routing function for the best output
  // interface's address
  Ipv6Header header;
  header.SetDestinationAddress (m_endPoint6->GetPeerAddress ());
  Socket::SocketErrno errno_;
  Ptr<Ipv6Route> route;
  Ptr<NetDevice> oif = m_socket->GetBoundNetDevice ();
  route = ipv6->GetRoutingProtocol ()->RouteOutput (Ptr<Packet> (), header, oif, errno_);
  if (route == 0)
    {
      NS_LOG_LOGIC ("Route to " << m_endPoint6->GetPeerAddress () << " does not exist");
      NS_LOG_ERROR (errno_);
      m_errno = errno_;
      return -1;
    }
  NS_LOG_LOGIC ("Route exists");
  m_endPoint6->SetLocalAddress (route->GetSource ());
  return 0;
}
/* Perform the real connection tasks: Send SYN if allowed, RST if invalid */
int
Ns3TcpSocketImpl::DoConnect (void)
{
  NS_LOG_FUNCTION (this);

  // A new connection is allowed only if this socket does not have a connection
  if (m_state == TcpSocket::CLOSED
      || m_state == TcpSocket::LISTEN
      || m_state == TcpSocket::SYN_SENT
      || m_state == TcpSocket::LAST_ACK
      || m_state == TcpSocket::CLOSE_WAIT)
    { // send a SYN packet and change state into SYN_SENT
      SendEmptyPacket (TcpHeader::SYN);
      NS_LOG_DEBUG (m_socket->TcpStateName[m_state] << " -> SYN_SENT");
      m_state = TcpSocket::SYN_SENT;
    }
  else if (m_state != TcpSocket::TIME_WAIT)
    { // In states SYN_RCVD, ESTABLISHED, FIN_WAIT_1, FIN_WAIT_2, and CLOSING, an connection
      // exists. We send RST, tear down everything, and close this socket.
      SendEmptyPacket (TcpHeader::RST);
      m_socket->NotifyErrorClose ();
      DeallocateEndPoint ();
      CloseAndNotify ();
    }
  return 0;
}
/* Inherit from Socket class: Send a packet. Parameter flags is not used.
    Packet has no TCP header. Invoked by upper-layer application */
int
Ns3TcpSocketImpl::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p);
  NS_ABORT_MSG_IF (flags, "use of flags is not supported in Ns3TcpSocketImpl::Send()");
  if (m_state == TcpSocket::ESTABLISHED
      || m_state == TcpSocket::SYN_SENT
      || m_state == TcpSocket::CLOSE_WAIT)
    {
      // Store the packet into Tx buffer
      if (!m_txBuffer->Add (p))
        { // TxBuffer overflow, send failed
          m_errno = TcpSocket::ERROR_MSGSIZE;
          return -1;
        }
      if (m_shutdownSend)
        {
          m_errno = TcpSocket::ERROR_SHUTDOWN;
          return -1;
        }
      // Submit the data to lower layers
      NS_LOG_LOGIC ("txBufSize=" << m_txBuffer->Size () << " state " <<
                    m_socket->TcpStateName[m_state]);
      if ((m_state == TcpSocket::ESTABLISHED
           || m_state == TcpSocket::CLOSE_WAIT) && AvailableWindow () > 0)
        { // Try to send the data out: Add a little step to allow the application
          // to fill the buffer
          if (!m_sendPendingDataEvent.IsRunning ())
            {
              m_sendPendingDataEvent = Simulator::Schedule (TimeStep (1),
                                                            &Ns3TcpSocketImpl::SendPendingData,
                                                            this, m_connected);
            }
        }
      return p->GetSize ();
    }
  else
    { // Connection not established yet
      m_errno = TcpSocket::ERROR_NOTCONN;
      return -1; // Send failure
    }
}

/* Inherit from Socket class: In Ns3TcpSocketImpl, it is same as Send() call */
int
Ns3TcpSocketImpl::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
  return Send (p, flags); // SendTo() and Send() are the same
}

// Note that this function did not implement the PSH flag
uint32_t
Ns3TcpSocketImpl::SendPendingData (bool withAck)
{
  NS_LOG_FUNCTION (this << withAck);
  if (m_txBuffer->Size () == 0)
    {
      return false;                           // Nothing to send
    }
  if (m_endPoint == 0 && m_endPoint6 == 0)
    {
      NS_LOG_INFO ("No endpoint; m_shutdownSend=" << m_shutdownSend);
      return false; // Is this the right way to handle this condition?
    }

  uint32_t nPacketsSent = 0;
  uint32_t availableWindow = AvailableWindow ();

  // RFC 6675, Section (C)
  // If cwnd - pipe >= 1 SMSS, the sender SHOULD transmit one or more
  // segments as follows:
  // (NOTE: We check > 0, and do the checks for segmentSize in the following
  // else branch to control silly window syndrome and Nagle)
  while (availableWindow > 0)
    {
      if (m_tcb->m_congState == TcpSocketState::CA_OPEN
          && m_state == TcpSocket::FIN_WAIT_1)
        {
          NS_LOG_INFO ("FIN_WAIT and OPEN state; no data to transmit");
          break;
        }
      // (C.1) The scoreboard MUST be queried via NextSeg () for the
      //       sequence number range of the next segment to transmit (if
      //       any), and the given segment sent.  If NextSeg () returns
      //       failure (no data to send), return without sending anything
      //       (i.e., terminate steps C.1 -- C.5).
      SequenceNumber32 next;
      if (!m_txBuffer->NextSeg (&next, m_retxThresh, m_tcb->m_segmentSize))
        {
          NS_LOG_INFO ("no valid seq to transmit, or no data available");
          break;
        }
      else
        {
          // It's time to transmit, but before do silly window and Nagle's check
          uint32_t availableData = m_txBuffer->SizeFromSequence (next);

          // Stop sending if we need to wait for a larger Tx window (prevent silly window syndrome)
          if (availableWindow < m_tcb->m_segmentSize &&  availableData > availableWindow)
            {
              NS_LOG_LOGIC ("Preventing Silly Window Syndrome. Wait to send.");
              break; // No more
            }
          // Nagle's algorithm (RFC896): Hold off sending if there is unacked data
          // in the buffer and the amount of data to send is less than one segment
          if (!m_noDelay && UnAckDataCount () > 0 && availableData < m_tcb->m_segmentSize)
            {
              NS_LOG_DEBUG ("Invoking Nagle's algorithm for seq " << next <<
                            ", SFS: " << m_txBuffer->SizeFromSequence (next) <<
                            ". Wait to send.");
              break;
            }

          uint32_t s = std::min (availableWindow, m_tcb->m_segmentSize);

          // (C.2) If any of the data octets sent in (C.1) are below HighData,
          //       HighRxt MUST be set to the highest sequence number of the
          //       retransmitted segment unless NextSeg () rule (4) was
          //       invoked for this retransmission.
          // (C.3) If any of the data octets sent in (C.1) are above HighData,
          //       HighData must be updated to reflect the transmission of
          //       previously unsent data.
          //
          // These steps are done in m_txBuffer with the tags.
          if (m_tracedValues.GetNextTxSequence () != next)
            {
              m_tracedValues.SetNextTxSequence (next);
            }

          uint32_t sz = SendDataPacket (m_tracedValues.GetNextTxSequence (), s, withAck);
          m_tracedValues.SetNextTxSequence (m_tracedValues.GetNextTxSequence () + sz);

          NS_LOG_LOGIC (" rxwin " << m_tracedValues.GetRWnd () <<
                        " segsize " << m_tcb->m_segmentSize <<
                        " highestRxAck " << m_txBuffer->HeadSequence () <<
                        " pd->Size " << m_txBuffer->Size () <<
                        " pd->SFS " << m_txBuffer->SizeFromSequence (m_tracedValues.GetNextTxSequence ()));

          NS_LOG_DEBUG ("cWnd: " << m_tracedValues.GetCWnd () <<
                        " total unAck: " << UnAckDataCount () <<
                        " sent seq " << m_tracedValues.GetNextTxSequence () <<
                        " size " << sz);

          ++nPacketsSent;
        }

      // (C.4) The estimate of the amount of data outstanding in the
      //       network must be updated by incrementing pipe by the number
      //       of octets transmitted in (C.1).
      //
      // Done in BytesInFlight, inside AvailableWindow.
      availableWindow = AvailableWindow ();

      // (C.5) If cwnd - pipe >= 1 SMSS, return to (C.1)
      // loop again!
    }

  if (nPacketsSent > 0)
    {
      NS_LOG_DEBUG ("SendPendingData sent " << nPacketsSent << " segments");
    }
  return nPacketsSent;
}

uint32_t
Ns3TcpSocketImpl::UnAckDataCount () const
{
  NS_LOG_FUNCTION (this);
  return m_tracedValues.GetHighTxMark () - m_txBuffer->HeadSequence ();
}

uint32_t
Ns3TcpSocketImpl::BytesInFlight () const
{
  NS_LOG_FUNCTION (this);
  // Previous (see bug 1783):
  // uint32_t bytesInFlight = m_highTxMark.Get () - m_txBuffer->HeadSequence ();
  // RFC 4898 page 23
  // PipeSize=SND.NXT-SND.UNA+(retransmits-dupacks)*CurMSS

  // flightSize == UnAckDataCount (), but we avoid the call to save log lines
  uint32_t bytesInFlight = m_txBuffer->BytesInFlight (m_retxThresh, m_tcb->m_segmentSize);

  // m_bytesInFlight is traced; avoid useless assignments which would fire
  // fruitlessly the callback
  if (m_tracedValues.GetBytesInFlight () != bytesInFlight)
    {
      // Ugly, but we are not modifying the state; m_bytesInFlight is used
      // only for tracing purpose.
      const_cast<Ns3TcpSocketImpl*> (this)->m_tracedValues.SetBytesInFlight (bytesInFlight);
    }

  return bytesInFlight;
}

uint32_t
Ns3TcpSocketImpl::Window (void) const
{
  NS_LOG_FUNCTION (this);
  return std::min (m_tracedValues.GetRWnd (), m_tracedValues.GetCWnd ());
}

uint32_t
Ns3TcpSocketImpl::AvailableWindow () const
{
  NS_LOG_FUNCTION_NOARGS ();
  uint32_t inflight = BytesInFlight (); // Number of outstanding bytes
  uint32_t win = Window ();             // Number of bytes allowed to be outstanding

  if (inflight > win)
    {
      NS_LOG_DEBUG ("InFlight=" << inflight << ", Win=" << win << " availWin=0");
      return 0;
    }

  NS_LOG_DEBUG ("InFlight=" << inflight << ", Win=" << win << " availWin=" << win-inflight);
  return win - inflight;
}
/* Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
    TCP header, and send to TcpL4Protocol */
uint32_t
Ns3TcpSocketImpl::SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck)
{
  NS_LOG_FUNCTION (this << seq << maxSize << withAck);

  bool isRetransmission = false;
  if (seq != m_tracedValues.GetHighTxMark ())
    {
      isRetransmission = true;
    }

  Ptr<Packet> p = m_txBuffer->CopyFromSequence (maxSize, seq);
  uint32_t sz = p->GetSize (); // Size of packet
  uint8_t flags = withAck ? TcpHeader::ACK : 0;
  uint32_t remainingData = m_txBuffer->SizeFromSequence (seq + SequenceNumber32 (sz));

  if (withAck)
    {
      m_delAckEvent.Cancel ();
      m_delAckCount = 0;
    }

  /*
   * Add tags for each socket option.
   * Note that currently the socket adds both IPv4 tag and IPv6 tag
   * if both options are set. Once the packet got to layer three, only
   * the corresponding tags will be read.
   */
  if (m_socket->GetIpTos ())
    {
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (m_socket->GetIpTos ());
      p->AddPacketTag (ipTosTag);
    }

  if (m_socket->IsManualIpv6Tclass ())
    {
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (m_socket->GetIpv6Tclass ());
      p->AddPacketTag (ipTclassTag);
    }

  if (m_socket->IsManualIpTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (m_socket->GetIpTtl ());
      p->AddPacketTag (ipTtlTag);
    }

  if (m_socket->IsManualIpv6HopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (m_socket->GetIpv6HopLimit ());
      p->AddPacketTag (ipHopLimitTag);
    }

  uint8_t priority = m_socket->GetPriority ();
  if (priority)
    {
      SocketPriorityTag priorityTag;
      priorityTag.SetPriority (priority);
      p->ReplacePacketTag (priorityTag);
    }

  if (m_closeOnEmpty && (remainingData == 0))
    {
      flags |= TcpHeader::FIN;
      if (m_state == TcpSocket::ESTABLISHED)
        { // On active close: I am the first one to send FIN
          NS_LOG_DEBUG ("ESTABLISHED -> FIN_WAIT_1");
          m_state = TcpSocket::FIN_WAIT_1;
        }
      else if (m_state == TcpSocket::CLOSE_WAIT)
        { // On passive close: Peer sent me FIN already
          NS_LOG_DEBUG ("CLOSE_WAIT -> LAST_ACK");
          m_state = TcpSocket::LAST_ACK;
        }
    }
  TcpHeader header;
  header.SetFlags (flags);
  header.SetSequenceNumber (seq);
  header.SetAckNumber (m_rxBuffer->NextRxSequence ());
  if (m_endPoint)
    {
      header.SetSourcePort (m_endPoint->GetLocalPort ());
      header.SetDestinationPort (m_endPoint->GetPeerPort ());
    }
  else
    {
      header.SetSourcePort (m_endPoint6->GetLocalPort ());
      header.SetDestinationPort (m_endPoint6->GetPeerPort ());
    }
  header.SetWindowSize (AdvertisedWindowSize ());
  AddOptions (header);

  if (m_retxEvent.IsExpired ())
    {
      // Schedules retransmit timeout. m_rto should be already doubled.

      NS_LOG_LOGIC (this << " SendDataPacket Schedule ReTxTimeout at time " <<
                    Simulator::Now ().GetSeconds () << " to expire at time " <<
                    (Simulator::Now () + m_tracedValues.GetRto ()).GetSeconds () );
      m_retxEvent = Simulator::Schedule (m_tracedValues.GetRto (), &Ns3TcpSocketImpl::ReTxTimeout, this);
    }

  m_tracedValues.TxTrace (p, header, m_socket);

  if (m_endPoint)
    {
      m_tcp->SendPacket (p, header, m_endPoint->GetLocalAddress (),
                         m_endPoint->GetPeerAddress (), m_socket->GetBoundNetDevice ());
      NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
                    remainingData << " via TcpL4Protocol to " <<  m_endPoint->GetPeerAddress () <<
                    ". Header " << header);
    }
  else
    {
      m_tcp->SendPacket (p, header, m_endPoint6->GetLocalAddress (),
                         m_endPoint6->GetPeerAddress (), m_socket->GetBoundNetDevice ());
      NS_LOG_DEBUG ("Send segment of size " << sz << " with remaining data " <<
                    remainingData << " via TcpL4Protocol to " <<  m_endPoint6->GetPeerAddress () <<
                    ". Header " << header);
    }

  UpdateRttHistory (seq, sz, isRetransmission);

  // Notify the application of the data being sent unless this is a retransmit
  if (seq + sz > m_tracedValues.GetHighTxMark())
    {
      Simulator::ScheduleNow (&TcpSocketBase::NotifyDataSent, m_socket,
                              (seq + sz - m_tracedValues.GetHighTxMark()));
    }
  // Update highTxMark
  m_tracedValues.SetHighTxMark (std::max(seq + sz, m_tracedValues.GetHighTxMark()));
  return sz;
}
// Retransmit timeout
void
Ns3TcpSocketImpl::ReTxTimeout ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
  // If erroneous timeout in closed/timed-wait state, just return
  if (m_state == TcpSocket::CLOSED || m_state == TcpSocket::TIME_WAIT)
    {
      return;
    }
  // If all data are received (non-closing socket and nothing to send), just return
  if (m_state <= TcpSocket::ESTABLISHED
      && m_txBuffer->HeadSequence () >= m_tracedValues.GetHighTxMark ())
    {
      return;
    }

  // From RFC 6675, Section 5.1
  // [RFC2018] suggests that a TCP sender SHOULD expunge the SACK
  // information gathered from a receiver upon a retransmission timeout
  // (RTO) "since the timeout might indicate that the data receiver has
  // reneged."  Additionally, a TCP sender MUST "ignore prior SACK
  // information in determining which data to retransmit."
  if (!m_sackEnabled)
    {
      // If SACK is not enabled, give up with all sack blocks we crafted
      // m_txBuffer->ResetScoreboard ();
      // And move all the sent packet into the unsent, again. (ResetScoreboard
      // is done inside that function)
      m_txBuffer->ResetSentList ();
    }
  else
    {
      // Continuing from RFC 6675, Section 5.1
      // It has been suggested that, as long as robust tests for
      // reneging are present, an implementation can retain and use SACK
      // information across a timeout event [Errata1610]

      // Please note that BytesInFlight should reflect the fact that all our
      // sent list is considered lost. The following line could be a start,
      // but we miss tests for reneging right now. So, be safe.
      // m_txBuffer->SetSentListLost ();
      m_txBuffer->ResetSentList ();
    }

  // From RFC 6675, Section 5.1
  // If an RTO occurs during loss recovery as specified in this document,
  // RecoveryPoint MUST be set to HighData.  Further, the new value of
  // RecoveryPoint MUST be preserved and the loss recovery algorithm
  // outlined in this document MUST be terminated.
  m_recover = m_tracedValues.GetHighTxMark ();

  // RFC 6298, clause 2.5, double the timer
  Time doubledRto = m_tracedValues.GetRto () * 2;
  m_tracedValues.SetRto (Min (doubledRto, Time::FromDouble (60,  Time::S)));

  // Empty RTT history
  m_history.clear ();

  // Reset dupAckCount
  m_dupAckCount = 0;

  // Please don't reset highTxMark, it is used for retransmission detection

  // When a TCP sender detects segment loss using the retransmission timer
  // and the given segment has not yet been resent by way of the
  // retransmission timer, decrease ssThresh
  if (m_tcb->m_congState != TcpSocketState::CA_LOSS || !m_txBuffer->IsHeadRetransmitted ())
    {
      m_tracedValues.SetSSThresh (m_congestionControl->GetSsThresh (m_tcb, BytesInFlight ()));
    }

  // Cwnd set to 1 MSS
  m_tracedValues.SetCwnd (m_tcb->m_segmentSize);

  m_congestionControl->CongestionStateSet (m_tcb, TcpSocketState::CA_LOSS);
  m_tcb->m_congState = TcpSocketState::CA_LOSS;

  NS_LOG_DEBUG ("RTO. Reset cwnd to " <<  m_tracedValues.GetCWnd () << ", ssthresh to " <<
                m_tracedValues.GetSSThresh () << ", restart from seqnum " <<
                m_txBuffer->HeadSequence () << " doubled rto to " <<
                m_tracedValues.GetRto().GetSeconds () << " s");

  NS_ASSERT_MSG (BytesInFlight () == 0, "There are some bytes in flight after an RTO: " <<
                 BytesInFlight ());

  // Retransmit the packet
  DoRetransmit ();

  NS_ASSERT_MSG (BytesInFlight () <= m_tcb->m_segmentSize,
                 "In flight there is more than one segment");
}
void
Ns3TcpSocketImpl::DoRetransmit ()
{
  NS_LOG_FUNCTION (this);
  // Retransmit SYN packet
  if (m_state == TcpSocket::SYN_SENT)
    {
      if (m_synCount > 0)
        {
          SendEmptyPacket (TcpHeader::SYN);
        }
      else
        {
          m_socket->NotifyConnectionFailed ();
        }
      return;
    }

  if (m_dataRetrCount == 0)
    {
      NS_LOG_INFO ("No more data retries available. Dropping connection");
      m_socket->NotifyErrorClose ();
      DeallocateEndPoint ();
      return;
    }
  else
    {
      --m_dataRetrCount;
    }

  // Retransmit non-data packet: Only if in FIN_WAIT_1 or CLOSING state
  if (m_txBuffer->Size () == 0)
    {
      if (m_state == TcpSocket::FIN_WAIT_1 || m_state == TcpSocket::CLOSING)
        { // Must have lost FIN, re-send
          SendEmptyPacket (TcpHeader::FIN);
        }
      return;
    }

  // Retransmit a data packet: Call SendDataPacket
  uint32_t sz = SendDataPacket (m_txBuffer->HeadSequence (), m_tcb->m_segmentSize, true);

  // In case of RTO, advance m_tcb->m_nextTxSequence
  m_tracedValues.SetNextTxSequence(std::max (m_tracedValues.GetNextTxSequence (),
                                   m_txBuffer->HeadSequence () + sz));

  NS_LOG_DEBUG ("retxing seq " << m_txBuffer->HeadSequence ());
}
} // namespace ns3
