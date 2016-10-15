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
#ifndef TCP_SOCKET_BASE_H
#define TCP_SOCKET_BASE_H

#include "tcp-socket.h"
#include "tcp-implementation.h"
#include "tcp-header.h"
#include "ipv6-header.h"
#include "ipv4-header.h"
#include "ipv4-interface.h"
#include "ipv6-interface.h"
#include "ns3/event-id.h"

namespace ns3 {

class TcpTxBuffer;
class TcpRxBuffer;

class TcpSocketBase : public TcpSocket
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the instance TypeId
   * \return the instance TypeId
   */
  virtual TypeId GetInstanceTypeId () const;

  TcpSocketBase ();
  virtual ~TcpSocketBase ();

  /**
   * \brief Called by the L3 protocol when it received a packet to pass on to TCP.
   *
   * \param packet the incoming packet
   * \param header the packet's IPv4 header
   * \param port the remote port
   * \param incomingInterface the incoming interface
   */
  void ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                  Ptr<Ipv4Interface> incomingInterface);

  /**
   * \brief Called by the L3 protocol when it received a packet to pass on to TCP.
   *
   * \param packet the incoming packet
   * \param header the packet's IPv6 header
   * \param port the remote port
   * \param incomingInterface the incoming interface
   */
  void ForwardUp6 (Ptr<Packet> packet, Ipv6Header header, uint16_t port,
                   Ptr<Ipv6Interface> incomingInterface);

  /**
   * \brief Called by TcpSocketBase::ForwardUp{,6}().
   *
   * Get a packet from L3. This is the real function to handle the
   * incoming packet from lower layers. This is
   * wrapped by ForwardUp() so that this function can be overloaded by daughter
   * classes.
   *
   * \param packet the incoming packet
   * \param fromAddress the address of the sender of packet
   * \param toAddress the address of the receiver of packet (hopefully, us)
   */
  void DoForwardUp (Ptr<Packet> packet, const Address &fromAddress,
                    const Address &toAddress);

  /**
   * \brief Called by the L3 protocol when it received an ICMP packet to pass on to TCP.
   *
   * \param icmpSource the ICMP source address
   * \param icmpTtl the ICMP Time to Live
   * \param icmpType the ICMP Type
   * \param icmpCode the ICMP Code
   * \param icmpInfo the ICMP Info
   */
  void ForwardIcmp (Ipv4Address icmpSource, uint8_t icmpTtl, uint8_t icmpType,
                    uint8_t icmpCode, uint32_t icmpInfo);

  /**
   * \brief Called by the L3 protocol when it received an ICMPv6 packet to pass on to TCP.
   *
   * \param icmpSource the ICMP source address
   * \param icmpTtl the ICMP Time to Live
   * \param icmpType the ICMP Type
   * \param icmpCode the ICMP Code
   * \param icmpInfo the ICMP Info
   */
  void ForwardIcmp6 (Ipv6Address icmpSource, uint8_t icmpTtl, uint8_t icmpType,
                     uint8_t icmpCode, uint32_t icmpInfo);

  /**
   * \brief Kill this socket by zeroing its attributes (IPv4)
   *
   * This is a callback function configured to m_endpoint in
   * SetupCallback(), invoked when the endpoint is destroyed.
   */
  void Destroy (void);

  /**
   * \brief Kill this socket by zeroing its attributes (IPv6)
   *
   * This is a callback function configured to m_endpoint in
   * SetupCallback(), invoked when the endpoint is destroyed.
   */
  void Destroy6 (void);

  void SetRttTypeId (const TypeId &typeId);
  void SetCongestionTypeId(const TypeId &typeId);
  void SetNode (const Ptr<Node> &node);
  void SetL4Protocol (const Ptr<TcpL4Protocol> &l4Protocol);


  /**
   * TracedCallback signature for tcp packet transmission or reception events.
   *
   * \param [in] packet The packet.
   * \param [in] ipv4
   * \param [in] interface
   */
  typedef void (* TcpTxRxTracedCallback)(const Ptr<const Packet> packet, const TcpHeader& header,
                                         const Ptr<const TcpSocketBase> socket);


  // Necessary implementations of null functions from ns3::Socket
  virtual enum SocketErrno GetErrno (void) const;    // returns m_errno
  virtual enum SocketType GetSocketType (void) const; // returns socket type
  virtual Ptr<Node> GetNode (void) const;            // returns m_node
  virtual int Bind (void);    // Bind a socket by setting up endpoint in TcpL4Protocol
  virtual int Bind6 (void);    // Bind a socket by setting up endpoint in TcpL4Protocol
  virtual int Bind (const Address &address);         // ... endpoint of specific addr or port
  virtual int Connect (const Address &address);      // Setup endpoint and call ProcessAction() to connect
  virtual int Listen (void);  // Verify the socket is in a correct state and call ProcessAction() to listen
  virtual int Close (void);   // Close by app: Kill socket upon tx buffer emptied
  virtual int ShutdownSend (void);    // Assert the m_shutdownSend flag to prevent send to network
  virtual int ShutdownRecv (void);    // Assert the m_shutdownRecv flag to prevent forward to app
  virtual int Send (Ptr<Packet> p, uint32_t flags);  // Call by app to send data to network
  virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress); // Same as Send(), toAddress is insignificant
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags); // Return a packet to be forwarded to app
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress); // ... and write the remote address at fromAddress
  virtual uint32_t GetTxAvailable (void) const; // Available Tx buffer size
  virtual uint32_t GetRxAvailable (void) const; // Available-to-read data size, i.e. value of m_rxAvailable
  virtual int GetSockName (Address &address) const; // Return local addr:port in address
  virtual int GetPeerName (Address &address) const;
  virtual void BindToNetDevice (Ptr<NetDevice> netdevice); // NetDevice with my m_endPoint

protected:
  // Implementing ns3::TcpSocket -- Attribute get/set
  // inherited, no need to doc
  virtual void     SetSndBufSize (uint32_t size);
  virtual uint32_t GetSndBufSize (void) const;
  virtual void     SetRcvBufSize (uint32_t size);
  virtual uint32_t GetRcvBufSize (void) const;
  virtual void     SetSegSize (uint32_t size);
  virtual uint32_t GetSegSize (void) const;
  virtual void     SetInitialSSThresh (uint32_t threshold);
  virtual uint32_t GetInitialSSThresh (void) const;
  virtual void     SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;
  virtual void     SetConnTimeout (Time timeout);
  virtual Time     GetConnTimeout (void) const;
  virtual void     SetSynRetries (uint32_t count);
  virtual uint32_t GetSynRetries (void) const;
  virtual void     SetDataRetries (uint32_t retries);
  virtual uint32_t GetDataRetries (void) const;
  virtual void     SetDelAckTimeout (Time timeout);
  virtual Time     GetDelAckTimeout (void) const;
  virtual void     SetDelAckMaxCount (uint32_t count);
  virtual uint32_t GetDelAckMaxCount (void) const;
  virtual void     SetTcpNoDelay (bool noDelay);
  virtual bool     GetTcpNoDelay (void) const;
  virtual void     SetPersistTimeout (Time timeout);
  virtual Time     GetPersistTimeout (void) const;
  virtual bool     SetAllowBroadcast (bool allowBroadcast);
  virtual bool     GetAllowBroadcast (void) const;

private:
  double GetMsl (void) const;
  void SetMsl (double msl);

  void SetMaxWinSize (uint16_t maxWinSize);
  uint16_t GetMaxWinSize () const;

  bool GetWinScaleEnabled () const;
  void SetWinScaleEnabled (bool enabled);

  bool GetSACKEnabled () const;
  void SetSACKEnabled (bool enabled);

  bool GetTimestampEnabled () const;
  void SetTimestampEnabled (bool enabled);

  void SetRxThresh (uint32_t rxThresh);
  uint32_t GetRxThresh () const;

  bool GetLimitedTx () const;
  void SetLimitedTx (bool enabled);

  bool GetMPTCPEnabled () const;
  void SetMPTCPEnabled (bool enabled);

  /**
   * \brief Sets the Minimum RTO.
   * \param minRto The minimum RTO.
   */
  void SetMinRto (Time minRto);

  /**
   * \brief Get the Minimum RTO.
   * \return The minimum RTO.
   */
  Time GetMinRto (void) const;

  /**
   * \brief Sets the Clock Granularity (used in RTO calcs).
   * \param clockGranularity The Clock Granularity
   */
  void SetClockGranularity (Time clockGranularity);

  /**
   * \brief Get the Clock Granularity (used in RTO calcs).
   * \return The Clock Granularity.
   */
  Time GetClockGranularity (void) const;

  /**
   * \brief Get a pointer to the Tx buffer
   * \return a pointer to the tx buffer
   */
  Ptr<TcpTxBuffer> GetTxBuffer (void) const;

  /**
   * \brief Get a pointer to the Rx buffer
   * \return a pointer to the rx buffer
   */
  Ptr<TcpRxBuffer> GetRxBuffer (void) const;

private:
  Ptr<TcpImplementation> m_implementation;
  // Connections to other layers of TCP/IP
  Callback<void, Ipv4Address,uint8_t,uint8_t,uint8_t,uint32_t> m_icmpCallback;  //!< ICMP callback
  Callback<void, Ipv6Address,uint8_t,uint8_t,uint8_t,uint32_t> m_icmpCallback6; //!< ICMPv6 callback
  TypeId m_rttTypeId;              //!< The RTT Estimator TypeId
  TypeId m_congestionTypeId;       //!< The socket TypeId
  Ptr<Node> m_node;  //!< The node
  Ptr<TcpL4Protocol> m_l4Protocol;

  TracedValue<Time> m_rto;             //!< Retransmit timeout
  TracedValue<Time> m_lastRtt;         //!< Last RTT sample collected
  TracedValue<uint32_t> m_rWnd;        //!< Receiver window (RCV.WND in RFC793)
  TracedValue<SequenceNumber32> m_highRxMark;     //!< Highest seqno received
  TracedValue<SequenceNumber32> m_highRxAckMark;  //!< Highest ack received
  TracedValue<uint32_t>         m_bytesInFlight; //!< Bytes in flight
  TracedValue<uint32_t>  m_cWnd;            //!< Congestion window
  TracedValue<uint32_t>  m_ssThresh;        //!< Slow start threshold
  TracedValue<SequenceNumber32> m_highTxMark; //!< Highest seqno ever sent, regardless of ReTx
  TracedValue<SequenceNumber32> m_nextTxSequence; //!< Next seqnum to be sent (SND.NXT), ReTx pushes it back

  // The following two traces pass a packet with a TCP header
  TracedCallback<Ptr<const Packet>, const TcpHeader&,
                 Ptr<const TcpSocketBase> > m_txTrace; //!< Trace of transmitted packets

  TracedCallback<Ptr<const Packet>, const TcpHeader&,
                 Ptr<const TcpSocketBase> > m_rxTrace; //!< Trace of received packets
};

} // namespace ns3

#endif // TCP_SOCKET_BASE_H
