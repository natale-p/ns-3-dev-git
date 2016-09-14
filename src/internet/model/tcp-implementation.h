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
#ifndef TCP_IMPLEMENTATION_H
#define TCP_IMPLEMENTATION_H

#include "ns3/object.h"
#include "ns3/socket.h"
#include "ns3/nstime.h"
#include "ns3/traced-value.h"
#include "ns3/sequence-number.h"

namespace ns3 {

class TcpL4Protocol;
class RttEstimator;
class TcpSocketBase;
class TcpCongestionOps;
class TcpTxBuffer;
class TcpRxBuffer;

struct TcpTracedValues
{
  TracedValue<Time> *m_rto;             //!< Retransmit timeout
  TracedValue<Time> *m_lastRtt;         //!< Last RTT sample collected
  TracedValue<uint32_t> *m_rWnd;        //!< Receiver window (RCV.WND in RFC793)
  TracedValue<SequenceNumber32> *m_highRxMark;     //!< Highest seqno received
  TracedValue<SequenceNumber32> *m_highRxAckMark;  //!< Highest ack received
  TracedValue<uint32_t>         *m_bytesInFlight; //!< Bytes in flight
  TracedValue<uint32_t>  *m_cWnd;            //!< Congestion window
  TracedValue<uint32_t>  *m_ssThresh;        //!< Slow start threshold
  TracedValue<SequenceNumber32> *m_highTxMark; //!< Highest seqno ever sent, regardless of ReTx
  TracedValue<SequenceNumber32> *m_nextTxSequence; //!< Next seqnum to be sent (SND.NXT), ReTx pushes it back
};

class TcpImplementation : public Object
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

  TcpImplementation ();
  virtual ~TcpImplementation ();

  virtual void Destroy () = 0;
  virtual void Destroy6 () = 0;

  virtual void SetTracedValues (const TcpTracedValues &traced) = 0;
  virtual void SetTcpSocket (Ptr<TcpSocketBase> socket) = 0;

  /**
   * \brief Set the associated node.
   * \param node the node
   */
  virtual void SetNode (Ptr<Node> node) = 0;

  /**
   * \brief Set the associated TCP L4 protocol.
   * \param tcp the TCP L4 protocol
   */
  virtual void SetL4Protocol (Ptr<TcpL4Protocol> tcp) = 0;

  /**
   * \brief Set the associated RTT estimator.
   * \param rtt the RTT estimator
   */
  virtual void SetRtt (Ptr<RttEstimator> rtt) = 0;

  /**
   * \brief Install a congestion control algorithm on this socket
   *
   * \param algo Algorithm to be installed
   */
  virtual void SetCongestionControlAlgorithm (Ptr<TcpCongestionOps> algo) = 0;

  virtual enum Socket::SocketErrno GetErrno (void) const = 0;
  virtual enum Socket::SocketType GetSocketType (void) const = 0;
  virtual Ptr<Node> GetNode (void) const = 0;
  virtual int Bind (void) = 0;
  virtual int Bind6 (void) = 0;
  virtual int Bind (const Address &address) = 0;
  virtual int Connect (const Address &address) = 0;
  virtual int Listen (void) = 0;
  virtual int Close (void) = 0;
  virtual int ShutdownSend (void) = 0;
  virtual int ShutdownRecv (void) = 0;
  virtual int Send (Ptr<Packet> p, uint32_t flags) = 0;
  virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress) = 0;
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags) = 0;
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress) = 0;
  virtual uint32_t GetTxAvailable (void) const = 0;
  virtual uint32_t GetRxAvailable (void) const = 0;
  virtual int GetSockName (Address &address) const = 0;
  virtual int GetPeerName (Address &address) const = 0;
  virtual void BindToNetDevice (Ptr<NetDevice> netdevice) = 0;

  virtual void     SetSndBufSize (uint32_t size) = 0;
  virtual uint32_t GetSndBufSize (void) const = 0;
  virtual void     SetRcvBufSize (uint32_t size) = 0;
  virtual uint32_t GetRcvBufSize (void) const = 0;
  virtual void     SetSegSize (uint32_t size) = 0;
  virtual uint32_t GetSegSize (void) const = 0;
  virtual void     SetInitialSSThresh (uint32_t threshold) = 0;
  virtual uint32_t GetInitialSSThresh (void) const = 0;
  virtual void     SetInitialCwnd (uint32_t cwnd) = 0;
  virtual uint32_t GetInitialCwnd (void) const = 0;
  virtual void     SetConnTimeout (Time timeout) = 0;
  virtual Time     GetConnTimeout (void) const = 0;
  virtual void     SetSynRetries (uint32_t count) = 0;
  virtual uint32_t GetSynRetries (void) const = 0;
  virtual void     SetDataRetries (uint32_t retries) = 0;
  virtual uint32_t GetDataRetries (void) const = 0;
  virtual void     SetDelAckTimeout (Time timeout) = 0;
  virtual Time     GetDelAckTimeout (void) const = 0;
  virtual void     SetDelAckMaxCount (uint32_t count) = 0;
  virtual uint32_t GetDelAckMaxCount (void) const = 0;
  virtual void     SetTcpNoDelay (bool noDelay) = 0;
  virtual bool     GetTcpNoDelay (void) const = 0;
  virtual void     SetPersistTimeout (Time timeout) = 0;
  virtual Time     GetPersistTimeout (void) const = 0;
  virtual bool     SetAllowBroadcast (bool allowBroadcast) = 0;
  virtual bool     GetAllowBroadcast (void) const = 0;

  virtual double GetMsl (void) const = 0;
  virtual void SetMsl (double msl) = 0;
  virtual void SetMaxWinSize (uint16_t maxWinSize) = 0;
  virtual uint16_t GetMaxWinSize () const = 0;
  virtual bool GetWinScaleEnabled () const = 0;
  virtual void SetWinScaleEnabled (bool enabled) = 0;
  virtual bool GetSACKEnabled () const = 0;
  virtual void SetSACKEnabled (bool enabled) = 0;
  virtual bool GetTimestampEnabled () const = 0;
  virtual void SetTimestampEnabled (bool enabled) = 0;
  virtual void SetRxThresh (uint32_t rxThresh) = 0;
  virtual uint32_t GetRxThresh () const = 0;
  virtual bool GetLimitedTx () const = 0;
  virtual void SetLimitedTx (bool enabled) = 0;

  virtual void SetMinRto (Time minRto) = 0;
  virtual Time GetMinRto (void) const = 0;
  virtual void SetClockGranularity (Time clockGranularity) = 0;
  virtual Time GetClockGranularity (void) const = 0;
  virtual Ptr<TcpTxBuffer> GetTxBuffer (void) const = 0;
  virtual Ptr<TcpRxBuffer> GetRxBuffer (void) const = 0;

  virtual uint16_t GetLocalPort () const = 0;
};

} // namespace ns3
#endif // TCP_IMPLEMENTATION_H
