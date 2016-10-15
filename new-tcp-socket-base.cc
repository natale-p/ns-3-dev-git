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
#include "tcp-socket-base.h"
#include "ns3-tcp-implementation.h"
#include "tcp-rx-buffer.h"
#include "tcp-tx-buffer.h"
#include "tcp-l4-protocol.h"
#include "tcp-congestion-ops.h"
#include "rtt-estimator.h"

#include "ns3/pointer.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpSocketBase");
NS_OBJECT_ENSURE_REGISTERED (TcpSocketBase);


TypeId
TcpSocketBase::GetInstanceTypeId () const
{
  return TcpSocketBase::GetTypeId ();
}

TypeId
TcpSocketBase::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpSocketBase")
    .SetParent<TcpSocket> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpSocketBase> ()
    .AddAttribute ("MaxSegLifetime",
                   "Maximum segment lifetime in seconds, use for TIME_WAIT state transition to CLOSED state",
                   DoubleValue (120), /* RFC793 says MSL=2 minutes*/
                   MakeDoubleAccessor (&TcpSocketBase::SetMsl,
                                       &TcpSocketBase::GetMsl),
                   MakeDoubleChecker<double> (0))
    .AddAttribute ("MaxWindowSize", "Max size of advertised window",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&TcpSocketBase::SetMaxWinSize,
                                         &TcpSocketBase::GetMaxWinSize),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("WindowScaling", "Enable or disable Window Scaling option",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpSocketBase::SetWinScaleEnabled,
                                        &TcpSocketBase::GetWinScaleEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("SACK", "Enable or disable SACK option",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::SetSACKEnabled,
                                        &TcpSocketBase::GetSACKEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("Timestamp", "Enable or disable Timestamp option",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpSocketBase::SetTimestampEnabled,
                                        &TcpSocketBase::GetTimestampEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("MPTCP", "Enable or disable MPTCP",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpSocketBase::SetMPTCPEnabled,
                                        &TcpSocketBase::GetMPTCPEnabled),
                   MakeBooleanChecker ())
    .AddAttribute ("MinRto",
                   "Minimum retransmit timeout value",
                   TimeValue (Seconds (1.0)), // RFC 6298 says min RTO=1 sec, but Linux uses 200ms.
                   // See http://www.postel.org/pipermail/end2end-interest/2004-November/004402.html
                   MakeTimeAccessor (&TcpSocketBase::SetMinRto,
                                     &TcpSocketBase::GetMinRto),
                   MakeTimeChecker ())
    .AddAttribute ("ClockGranularity",
                   "Clock Granularity used in RTO calculations",
                   TimeValue (MilliSeconds (1)), // RFC6298 suggest to use fine clock granularity
                   MakeTimeAccessor (&TcpSocketBase::SetClockGranularity,
                                     &TcpSocketBase::GetClockGranularity),
                   MakeTimeChecker ())
    .AddAttribute ("TxBuffer",
                   "TCP Tx buffer",
                   PointerValue (),
                   MakePointerAccessor (&TcpSocketBase::GetTxBuffer),
                   MakePointerChecker<TcpTxBuffer> ())
    .AddAttribute ("RxBuffer",
                   "TCP Rx buffer",
                   PointerValue (),
                   MakePointerAccessor (&TcpSocketBase::GetRxBuffer),
                   MakePointerChecker<TcpRxBuffer> ())
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                   UintegerValue (3),
                   MakeUintegerAccessor (&TcpSocketBase::SetRxThresh,
                                         &TcpSocketBase::GetRxThresh),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
                   BooleanValue (true),
                   MakeBooleanAccessor (&TcpSocketBase::SetLimitedTx,
                                        &TcpSocketBase::GetLimitedTx),
                   MakeBooleanChecker ())
    .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&TcpSocketBase::m_icmpCallback),
                   MakeCallbackChecker ())
    .AddAttribute ("IcmpCallback6", "Callback invoked whenever an icmpv6 error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&TcpSocketBase::m_icmpCallback6),
                   MakeCallbackChecker ())
    .AddTraceSource ("RTO",
                     "Retransmission timeout",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rto),
                     "ns3::Time::TracedValueCallback")
    .AddTraceSource ("RTT",
                     "Last RTT sample",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_lastRtt),
                     "ns3::Time::TracedValueCallback")
    .AddTraceSource ("NextTxSequence",
                     "Next sequence number to send (SND.NXT)",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_nextTxSequence),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("HighestSequence",
                     "Highest sequence number ever sent in socket's life time",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highTxMark),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("RWND",
                     "Remote side's flow control window",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rWnd),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("BytesInFlight",
                     "Socket estimation of bytes in flight",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_bytesInFlight),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("HighestRxSequence",
                     "Highest sequence number received from peer",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highRxMark),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("HighestRxAck",
                     "Highest ack received from peer",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_highRxAckMark),
                     "ns3::SequenceNumber32TracedValueCallback")
    .AddTraceSource ("CongestionWindow",
                     "The TCP connection's congestion window",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_cWnd),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("SlowStartThreshold",
                     "TCP slow start threshold (bytes)",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_ssThresh),
                     "ns3::TracedValueCallback::Uint32")
    .AddTraceSource ("Tx",
                     "Send tcp packet to IP protocol",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_txTrace),
                     "ns3::TcpSocketBase::TcpTxRxTracedCallback")
    .AddTraceSource ("Rx",
                     "Receive tcp packet from IP protocol",
                     MakeTraceSourceAccessor (&TcpSocketBase::m_rxTrace),
                     "ns3::TcpSocketBase::TcpTxRxTracedCallback")
  ;
  return tid;
}

TcpSocketBase::TcpSocketBase()
  : TcpSocket ()
{
}

TcpSocketBase::~TcpSocketBase ()
{
}

enum Socket::SocketErrno
TcpSocketBase::GetErrno (void) const
{
  return m_implementation->GetErrno ();
}
enum Socket::SocketType
TcpSocketBase::GetSocketType (void) const
{
  return m_implementation->GetSocketType ();
}
Ptr<Node>
TcpSocketBase::GetNode (void) const
{
  return m_implementation->GetNode ();
}
int
TcpSocketBase::Bind (void)
{
  return m_implementation->Bind ();
}
int
TcpSocketBase::Bind6 (void)
{
  return m_implementation->Bind6 ();
}
int
TcpSocketBase::Bind (const Address &address)
{
  InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
  SetIpTos (transport.GetTos ());

  return m_implementation->Bind (address);
}
int
TcpSocketBase::Connect (const Address &address)
{
  return m_implementation->Connect (address);
}
int
TcpSocketBase::Listen (void)
{
  return m_implementation->Listen ();
}
int
TcpSocketBase::Close (void)
{
  return m_implementation->Close ();
}
int
TcpSocketBase::ShutdownSend (void)
{
  return m_implementation->ShutdownSend ();
}
int
TcpSocketBase::ShutdownRecv (void)
{
  return m_implementation->ShutdownRecv ();
}
int
TcpSocketBase::Send (Ptr<Packet> p, uint32_t flags)
{
  return m_implementation->Send (p, flags);
}
int
TcpSocketBase::SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress)
{
  return m_implementation->SendTo (p, flags, toAddress);
}
Ptr<Packet>
TcpSocketBase::Recv (uint32_t maxSize, uint32_t flags)
{
  return m_implementation->Recv (maxSize, flags);
}
Ptr<Packet>
TcpSocketBase::RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress)
{
  return m_implementation->RecvFrom (maxSize, flags, fromAddress);
}
uint32_t
TcpSocketBase::GetTxAvailable (void) const
{
  return m_implementation->GetTxAvailable ();
}
uint32_t
TcpSocketBase::GetRxAvailable (void) const
{
  return m_implementation->GetRxAvailable ();
}
int
TcpSocketBase::GetSockName (Address &address) const
{
  return m_implementation->GetSockName (address);
}
int
TcpSocketBase::GetPeerName (Address &address) const
{
  return m_implementation->GetPeerName (address);
}
void
TcpSocketBase::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  Socket::BindToNetDevice (netdevice); // Includes sanity check
  m_implementation->BindToNetDevice (netdevice);
}
void
TcpSocketBase::SetSndBufSize (uint32_t size)
{
  m_implementation->SetSndBufSize (size);
}
uint32_t
TcpSocketBase::GetSndBufSize (void) const
{
  return m_implementation->GetSndBufSize ();
}
void
TcpSocketBase::SetRcvBufSize (uint32_t size)
{
  m_implementation->SetRcvBufSize (size);
}
uint32_t
TcpSocketBase::GetRcvBufSize (void) const
{
  return m_implementation->GetRcvBufSize ();
}
void
TcpSocketBase::SetSegSize (uint32_t size)
{
  m_implementation->SetSegSize (size);
}
uint32_t
TcpSocketBase::GetSegSize (void) const
{
  return m_implementation->GetSegSize ();
}
void
TcpSocketBase::SetInitialSSThresh (uint32_t threshold)
{
  m_implementation->SetInitialSSThresh (threshold);
}
uint32_t
TcpSocketBase::GetInitialSSThresh (void) const
{
  return m_implementation->GetInitialSSThresh ();
}
void
TcpSocketBase::SetInitialCwnd (uint32_t cwnd)
{
  m_implementation->SetInitialCwnd (cwnd);
}
uint32_t
TcpSocketBase::GetInitialCwnd (void) const
{
  return m_implementation->GetInitialCwnd ();
}
void
TcpSocketBase::SetConnTimeout (Time timeout)
{
  m_implementation->SetConnTimeout (timeout);
}
Time
TcpSocketBase::GetConnTimeout (void) const
{
  return m_implementation->GetConnTimeout ();
}
void
TcpSocketBase::SetSynRetries (uint32_t count)
{
  m_implementation->SetSynRetries (count);
}
uint32_t
TcpSocketBase::GetSynRetries (void) const
{
  return m_implementation->GetSynRetries ();
}
void
TcpSocketBase::SetDataRetries (uint32_t retries)
{
  m_implementation->SetDataRetries (retries);
}
uint32_t
TcpSocketBase::GetDataRetries (void) const
{
  return m_implementation->GetDataRetries ();
}
void
TcpSocketBase::SetDelAckTimeout (Time timeout)
{
  m_implementation->SetDelAckTimeout (timeout);
}
Time
TcpSocketBase::GetDelAckTimeout (void) const
{
  return m_implementation->GetDelAckTimeout ();
}
void
TcpSocketBase::SetDelAckMaxCount (uint32_t count)
{
  m_implementation->SetDelAckMaxCount (count);
}
uint32_t
TcpSocketBase::GetDelAckMaxCount (void) const
{
  return m_implementation->GetDelAckMaxCount ();
}
void
TcpSocketBase::SetTcpNoDelay (bool noDelay)
{
  m_implementation->SetTcpNoDelay (noDelay);
}
bool
TcpSocketBase::GetTcpNoDelay (void) const
{
  return m_implementation->GetTcpNoDelay ();
}
void
TcpSocketBase::SetPersistTimeout (Time timeout)
{
  return m_implementation->SetPersistTimeout (timeout);
}
Time
TcpSocketBase::GetPersistTimeout (void) const
{
  return m_implementation->GetPersistTimeout ();
}
bool
TcpSocketBase::SetAllowBroadcast (bool allowBroadcast)
{
  return m_implementation->SetAllowBroadcast (allowBroadcast);
}
bool
TcpSocketBase::GetAllowBroadcast (void) const
{
  return m_implementation->GetAllowBroadcast ();
}
double
TcpSocketBase::GetMsl (void) const
{
  return m_implementation->GetMsl ();
}
void
TcpSocketBase::SetMsl (double msl)
{
  m_implementation->SetMsl (msl);
}
void
TcpSocketBase::SetMaxWinSize (uint16_t maxWinSize)
{
  m_implementation->SetMaxWinSize (maxWinSize);
}
uint16_t
TcpSocketBase::GetMaxWinSize () const
{
  return m_implementation->GetMaxWinSize ();
}

bool
TcpSocketBase::GetWinScaleEnabled () const
{
  return m_implementation->GetWinScaleEnabled ();
}
void
TcpSocketBase::SetWinScaleEnabled (bool enabled)
{
  m_implementation->SetWinScaleEnabled (enabled);
}

bool
TcpSocketBase::GetSACKEnabled () const
{
  return m_implementation->GetSACKEnabled ();
}
void
TcpSocketBase::SetSACKEnabled (bool enabled)
{
  m_implementation->SetSACKEnabled (enabled);
}

bool
TcpSocketBase::GetTimestampEnabled () const
{
  return m_implementation->GetTimestampEnabled ();
}
void
TcpSocketBase::SetTimestampEnabled (bool enabled)
{
  m_implementation->SetTimestampEnabled (enabled);
}

void
TcpSocketBase::SetRxThresh (uint32_t rxThresh)
{
  m_implementation->SetRxThresh (rxThresh);
}
uint32_t
TcpSocketBase::GetRxThresh () const
{
  return m_implementation->GetRxThresh ();
}

bool
TcpSocketBase::GetLimitedTx () const
{
  return m_implementation->GetLimitedTx ();
}
void
TcpSocketBase::SetLimitedTx (bool enabled)
{
  m_implementation->SetLimitedTx (enabled);
}

void
TcpSocketBase::SetMinRto (Time minRto)
{
  m_implementation->SetMinRto (minRto);
}

Time
TcpSocketBase::GetMinRto (void) const
{
  return m_implementation->GetMinRto ();
}
void
TcpSocketBase::SetClockGranularity (Time clockGranularity)
{
  m_implementation->SetClockGranularity (clockGranularity);
}

Time
TcpSocketBase::GetClockGranularity (void) const
{
  return m_implementation->GetClockGranularity ();
}

Ptr<TcpTxBuffer>
TcpSocketBase::GetTxBuffer (void) const
{
  return m_implementation->GetTxBuffer ();
}

Ptr<TcpRxBuffer>
TcpSocketBase::GetRxBuffer (void) const
{
  return m_implementation->GetRxBuffer ();
}

/* Function called by the L3 protocol when it received a packet to pass on to
    the TCP. This function is registered as the "RxCallback" function in
    SetupCallback(), which invoked by Bind(), and CompleteFork() */
void
TcpSocketBase::ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
{
  Address fromAddress = InetSocketAddress (header.GetSource (), port);
  Address toAddress = InetSocketAddress (header.GetDestination (),
                                         m_implementation->GetLocalPort ());

  DoForwardUp (packet, fromAddress, toAddress);
}


void
TcpSocketBase::ForwardUp6 (Ptr<Packet> packet, Ipv6Header header, uint16_t port,
                           Ptr<Ipv6Interface> incomingInterface)
{
  Address fromAddress = Inet6SocketAddress (header.GetSourceAddress (), port);
  Address toAddress = Inet6SocketAddress (header.GetDestinationAddress (),
                                          m_implementation->GetLocalPort ());

  DoForwardUp (packet, fromAddress, toAddress);
}

void
TcpSocketBase::ForwardIcmp (Ipv4Address icmpSource, uint8_t icmpTtl,
                            uint8_t icmpType, uint8_t icmpCode,
                            uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback.IsNull ())
    {
      m_icmpCallback (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void
TcpSocketBase::ForwardIcmp6 (Ipv6Address icmpSource, uint8_t icmpTtl,
                             uint8_t icmpType, uint8_t icmpCode,
                             uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback6.IsNull ())
    {
      m_icmpCallback6 (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void
TcpSocketBase::Destroy (void)
{
  // Maybe we should do something here? If not, just delete these function
  // and set the callback inside the TcpImplementation
  m_implementation->Destroy ();
}

void
TcpSocketBase::Destroy6 (void)
{
  // Maybe we should do something here? If not, just delete these function
  // and set the callback inside the TcpImplementation
  m_implementation->Destroy6 ();
}

void
TcpSocketBase::SetRttTypeId (const TypeId &typeId)
{
  m_rttTypeId = typeId;
}

void
TcpSocketBase::SetCongestionTypeId(const TypeId &typeId)
{
  m_congestionTypeId = typeId;
}

void
TcpSocketBase::SetNode (const Ptr<Node> &node)
{
  m_node = node;
}

void
TcpSocketBase::SetL4Protocol (const Ptr<TcpL4Protocol> &l4Protocol)
{
  m_l4Protocol = l4Protocol;
}

bool
TcpSocketBase::GetMPTCPEnabled () const
{
  return m_implementation->GetInstanceTypeId() != Ns3TcpImplementation::GetTypeId ();
}

void
TcpSocketBase::SetMPTCPEnabled (bool enabled)
{
  ObjectFactory rttFactory;
  ObjectFactory congAlgoFactory;
  rttFactory.SetTypeId (m_rttTypeId);
  congAlgoFactory.SetTypeId (m_congestionTypeId);

  struct TcpTracedValues values;
  values.m_bytesInFlight = &m_bytesInFlight;
  values.m_cWnd = &m_cWnd;
  values.m_highRxAckMark = &m_highRxAckMark;
  values.m_highRxMark = &m_highRxMark;
  values.m_highTxMark = &m_highTxMark;
  values.m_lastRtt = &m_lastRtt;
  values.m_nextTxSequence = &m_nextTxSequence;
  values.m_rto = &m_rto;
  values.m_rWnd = &m_rWnd;
  values.m_ssThresh = &m_ssThresh;

  if (enabled)
    {
      NS_FATAL_ERROR ("Not implemented yet");
    }
  else
    {
      m_implementation = CreateObject <Ns3TcpImplementation> ();
      m_implementation->SetRtt(DynamicCast<RttEstimator> (rttFactory.Create ()));
      m_implementation->SetCongestionControlAlgorithm(DynamicCast<TcpCongestionOps> (congAlgoFactory.Create()));
      m_implementation->SetTracedValues (values);
    }
}

} // namespace ns3
