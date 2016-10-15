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
#ifndef NS3_TCP_IMPLEMENTATION_H
#define NS3_TCP_IMPLEMENTATION_H

#include "tcp-implementation.h"
#include "tcp-socket.h"
#include "tcp-rtt-history.h"
#include "tcp-socket-state.h"
#include "tcp-header.h"
#include "ns3/event-id.h"

namespace ns3 {

class Ipv4EndPoint;
class Ipv6EndPoint;

/**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief A base class for implementation of a stream socket using TCP.
 *
 * This class contains the essential components of TCP, as well as a sockets
 * interface for upper layers to call. This class provides connection orientation
 * and sliding window flow control; congestion control is delegated to subclasses
 * of TcpCongestionOps. Part of TcpSocketBase is modified from the original
 * NS-3 TCP socket implementation (TcpSocketImpl) by
 * Raj Bhattacharjea <raj.b@gatech.edu> of Georgia Tech.
 *
 * For IPv4 packets, the TOS set for the socket is used. The Bind and Connect
 * operations set the TOS for the socket to the value specified in the provided
 * address. A SocketIpTos tag is only added to the packet if the resulting
 * TOS is non-null.
 * Each packet is assigned the priority set for the socket. Setting a TOS
 * for a socket also sets a priority for the socket (according to the
 * Socket::IpTos2Priority function). A SocketPriority tag is only added to the
 * packet if the priority is non-null.
 *
 * Congestion state machine
 * ---------------------------
 *
 * The socket maintains two state machines; the TCP one, and another called
 * "Congestion state machine", which keeps track of the phase we are in. Currently,
 * ns-3 manages the states:
 *
 * - CA_OPEN
 * - CA_DISORDER
 * - CA_RECOVERY
 * - CA_LOSS
 *
 * Another one (CA_CWR) is present but not used. For more information, see
 * the TcpCongState_t documentation.
 *
 * Congestion control interface
 * ---------------------------
 *
 * Congestion control, unlike older releases of ns-3, has been splitted from
 * TcpSocketBase. In particular, each congestion control is now a subclass of
 * the main TcpCongestionOps class. Switching between congestion algorithm is
 * now a matter of setting a pointer into the TcpSocketBase class. The idea
 * and the interfaces are inspired by the Linux operating system, and in
 * particular from the structure tcp_congestion_ops.
 *
 * Transmission Control Block (TCB)
 * --------------------------------
 *
 * The variables needed to congestion control classes to operate correctly have
 * been moved inside the TcpSocketState class. It contains information on the
 * congestion window, slow start threshold, segment size and the state of the
 * Congestion state machine.
 *
 * To track the trace inside the TcpSocketState class, a "forward" technique is
 * used, which consists in chaining callbacks from TcpSocketState to TcpSocketBase
 * (see for example cWnd trace source).
 *
 * Fast retransmit
 * ----------------
 *
 * The fast retransmit enhancement is introduced in RFC 2581 and updated in
 * RFC 5681. It basically reduces the time a sender waits before retransmitting
 * a lost segment, through the assumption that if it receives a certain number
 * of duplicate ACKs, a segment has been lost and it can be retransmitted.
 * Usually it is coupled with the Limited Transmit algorithm, defined in
 * RFC 3042.
 *
 * In ns-3, these algorithms are included in this class, and it is implemented inside
 * the ProcessAck method. The attribute which manages the number of dup ACKs
 * necessary to start the fast retransmit algorithm is named "ReTxThreshold",
 * and its default value is 3, while the Limited Transmit one can be enabled
 * by setting the attribute "LimitedTransmit" to true. Before entering the
 * recovery phase, the method EnterRecovery is called.
 *
 * Fast recovery
 * -------------
 *
 * The fast recovery algorithm is introduced RFC 2001, and it basically
 * avoids to reset cWnd to 1 segment after sensing a loss on the channel. Instead,
 * the slow start threshold is halved, and the cWnd is set equal to such value,
 * plus segments for the cWnd inflation.
 *
 * The algorithm is implemented in the ProcessAck method.
 *
 * RTO expiration
 * --------------
 *
 * When the Retransmission Time Out expires, the TCP faces a big performance
 * drop. The expiration event is managed in ReTxTimeout method, that basically
 * set the cWnd to 1 segment and start "from scratch" again.
 *
 * Options management
 * ------------------
 *
 * SYN and SYN-ACK options, that are allowed only at the beginning of the
 * connection, are managed in the DoForwardUp and SendEmptyPacket methods.
 * To read all others, we have setup a cycle inside ReadOptions. For adding
 * them, there is no a unique place, since the options (and the informations
 * available to build them) are scattered around the code. For instance,
 * the SACK option is built in SendEmptyPacket only under a certain conditions.
 *
 * SACK
 * ----
 *
 * The SACK generation/management is delegated to the buffer classes, namely
 * TcpTxBuffer and TcpRxBuffer. Please take a look on their documentation if
 * you need more informations.
 *
 */
class Ns3TcpSocketImpl : public TcpImplementation
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

  Ns3TcpSocketImpl ();
  ~Ns3TcpSocketImpl ();

  virtual void Destroy ();
  virtual void Destroy6 ();
  virtual void SetTcpSocket(Ptr<TcpSocketBase> socket);
  virtual void SetNode (Ptr<Node> node);
  virtual void SetL4Protocol (Ptr<TcpL4Protocol> tcp);
  virtual void SetRtt (Ptr<RttEstimator> rtt);

  virtual enum Socket::SocketErrno GetErrno (void) const;
  virtual enum Socket::SocketType GetSocketType (void) const;
  virtual Ptr<Node> GetNode (void) const;
  virtual int Bind (void);
  virtual int Bind6 (void);
  virtual int Bind (const Address &address);
  virtual int Connect (const Address &address);
  virtual int Listen (void);
  virtual int Close (void);
  virtual int ShutdownSend (void);
  virtual int ShutdownRecv (void);
  virtual int Send (Ptr<Packet> p, uint32_t flags);
  virtual int SendTo (Ptr<Packet> p, uint32_t flags, const Address &toAddress);
  virtual Ptr<Packet> Recv (uint32_t maxSize, uint32_t flags);
  virtual Ptr<Packet> RecvFrom (uint32_t maxSize, uint32_t flags, Address &fromAddress);
  virtual uint32_t GetTxAvailable (void) const;
  virtual uint32_t GetRxAvailable (void) const;
  virtual int GetSockName (Address &address) const;
  virtual int GetPeerName (Address &address) const;
  virtual void BindToNetDevice (Ptr<NetDevice> netdevice);

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

  virtual void SetTracedValues (const TcpTracedValues &traced);

  virtual double GetMsl (void) const;
  virtual void SetMsl (double msl);

  virtual void SetMaxWinSize (uint16_t maxWinSize);
  virtual uint16_t GetMaxWinSize () const;

  virtual bool GetWinScaleEnabled () const;
  virtual void SetWinScaleEnabled (bool enabled);

  virtual bool GetSACKEnabled () const;
  virtual void SetSACKEnabled (bool enabled);

  virtual bool GetTimestampEnabled () const;
  virtual void SetTimestampEnabled (bool enabled);

  virtual void SetRxThresh (uint32_t rxThresh);
  virtual uint32_t GetRxThresh () const;

  virtual bool GetLimitedTx () const;
  virtual void SetLimitedTx (bool enabled);

  virtual void SetMinRto (Time minRto);
  virtual Time GetMinRto (void) const;
  virtual void SetClockGranularity (Time clockGranularity);
  virtual Time GetClockGranularity (void) const;
  virtual Ptr<TcpTxBuffer> GetTxBuffer (void) const;
  virtual Ptr<TcpRxBuffer> GetRxBuffer (void) const;

  virtual uint16_t GetLocalPort() const;
  virtual void SetCongestionControlAlgorithm (Ptr<TcpCongestionOps> algo);

private:
  /**
   * \brief Common part of the two Bind(), i.e. set callback and remembering local addr:port
   *
   * \returns 0 on success, -1 on failure
   */
  int SetupCallback (void);
  /**
   * \brief Cancel all timer when endpoint is deleted
   */
  void CancelAllTimers (void);
  /**
   * \brief Send a empty packet that carries a flag, e.g. ACK
   *
   * \param flags the packet's flags
   */
  void SendEmptyPacket (uint8_t flags);
  /** \brief Add options to TcpHeader
   *
   * Test each option, and if it is enabled on our side, add it
   * to the header
   *
   * \param tcpHeader TcpHeader to add options to
   */
  void AddOptions (TcpHeader& tcpHeader);
  /**
   * \brief Return true if the specified option is enabled
   *
   * \param kind kind of TCP option
   * \return true if the option is enabled
   */
  bool IsTcpOptionEnabled (uint8_t kind) const;

  /**
   * \brief Read and parse the Window scale option
   *
   * Read the window scale option (encoded logarithmically) and save it.
   * Per RFC 1323, the value can't exceed 14.
   *
   * \param option Window scale option read from the header
   */
  void ProcessOptionWScale (const Ptr<const TcpOption> option);
  /**
   * \brief Add the window scale option to the header
   *
   * Calculate our factor from the rxBuffer max size, and add it
   * to the header.
   *
   * \param header TcpHeader where the method should add the window scale option
   */
  void AddOptionWScale (TcpHeader& header);

  /**
   * \brief Calculate window scale value based on receive buffer space
   *
   * Calculate our factor from the rxBuffer max size
   *
   * \returns the Window Scale factor
   */
  uint8_t CalculateWScale () const;

  /**
   * \brief Read the SACK PERMITTED option
   *
   * Currently this is a placeholder, since no operations should be done
   * on such option.
   *
   * \param SACK PERMITTED option from the header
   */
  void ProcessOptionSACKPermitted (const Ptr<const TcpOption> option);

  /**
   * \brief Read the SACK option
   *
   * \param SACK option from the header
   */
  bool ProcessOptionSACK (const Ptr<const TcpOption> option);

  /**
   * \brief Add the SACK PERMITTED option to the header
   *
   * \param header TcpHeader where the method should add the option
   */
  void AddOptionSACKPermitted (TcpHeader &header);

  /**
   * \brief Add the SACK option to the header
   *
   * \param header TcpHeader where the method should add the option
   */
  void AddOptionSACK (TcpHeader& header);

  /** \brief Process the timestamp option from other side
   *
   * Get the timestamp and the echo, then save timestamp (which will
   * be the echo value in our out-packets) and save the echoed timestamp,
   * to utilize later to calculate RTT.
   *
   * \see EstimateRtt
   * \param option Option from the segment
   * \param seq Sequence number of the segment
   */
  void ProcessOptionTimestamp (const Ptr<const TcpOption> option,
                               const SequenceNumber32 &seq);
  /**
   * \brief Add the timestamp option to the header
   *
   * Set the timestamp as the lower bits of the Simulator::Now time,
   * and the echo value as the last seen timestamp from the other part.
   *
   * \param header TcpHeader to which add the option to
   */
  void AddOptionTimestamp (TcpHeader& header);
  /**
   * \brief The amount of Rx window announced to the peer
   * \param scale indicate if the window should be scaled. True for
   * almost all cases, except when we are sending a SYN
   * \returns size of Rx window announced to the peer
   */
  uint16_t AdvertisedWindowSize (bool scale = true) const;
  /**
   * \brief Peacefully close the socket by notifying the upper layer and deallocate end point
   */
  void CloseAndNotify (void);
  /**
   * \brief Update the RTT history, when we send TCP segments
   *
   * \param seq The sequence number of the TCP segment
   * \param sz The segment's size
   * \param isRetransmission Whether or not the segment is a retransmission
   */

  void UpdateRttHistory (const SequenceNumber32 &seq, uint32_t sz,
                         bool isRetransmission);

  /**
   * \brief Deallocate m_endPoint and m_endPoint6
   */
  void DeallocateEndPoint (void);
  /**
   * \brief Configure the endpoint to a local address. Called by Connect() if Bind() didn't specify one.
   *
   * \returns 0 on success
   */
  int SetupEndpoint (void);

  /**
   * \brief Configure the endpoint v6 to a local address. Called by Connect() if Bind() didn't specify one.
   *
   * \returns 0 on success
   */
  int SetupEndpoint6 (void);
  /**
   * \brief Perform the real connection tasks: Send SYN if allowed, RST if invalid
   *
   * \returns 0 on success
   */
  int DoConnect (void);
  // Window management

  /**
   * \brief Return count of number of unacked bytes
   *
   * The difference between SND.UNA and HighTx
   *
   * \returns count of number of unacked bytes
   */
  virtual uint32_t UnAckDataCount (void) const;

  /**
   * \brief Return total bytes in flight
   *
   * Does not count segments lost and SACKed (or dupACKed)
   *
   * \returns total bytes in flight
   */
  virtual uint32_t BytesInFlight (void) const;

  /**
   * \brief Return the max possible number of unacked bytes
   * \returns the max possible number of unacked bytes
   */
  virtual uint32_t Window (void) const;

  /**
   * \brief Return unfilled portion of window
   * \return unfilled portion of window
   */
  virtual uint32_t AvailableWindow (void) const;

  /**
   * \brief Send as much pending data as possible according to the Tx window.
   *
   * Note that this function did not implement the PSH flag.
   *
   * \param withAck forces an ACK to be sent
   * \returns the number of packets sent
   */
  uint32_t SendPendingData (bool withAck = false);
  /**
   * \brief Extract at most maxSize bytes from the TxBuffer at sequence seq, add the
   *        TCP header, and send to TcpL4Protocol
   *
   * \param seq the sequence number
   * \param maxSize the maximum data block to be transmitted (in bytes)
   * \param withAck forces an ACK to be sent
   * \returns the number of bytes sent
   */
  uint32_t SendDataPacket (SequenceNumber32 seq, uint32_t maxSize, bool withAck);
  /**
   * \brief An RTO event happened
   */
  void ReTxTimeout (void);
  /**
   * \brief Retransmit the oldest packet
   */
  void DoRetransmit (void);


private:
  Ptr<TcpSocketBase> m_socket;
  TcpTracedValues m_tracedValues;

  // Counters and events
  EventId           m_retxEvent;       //!< Retransmission event
  EventId           m_lastAckEvent;    //!< Last ACK timeout event
  EventId           m_delAckEvent;     //!< Delayed ACK timeout event
  EventId           m_persistEvent;    //!< Persist event: Send 1 byte to probe for a non-zero Rx window
  EventId           m_timewaitEvent;   //!< TIME_WAIT expiration event: Move this socket to CLOSED state
  uint32_t          m_dupAckCount;     //!< Dupack counter
  uint32_t          m_delAckCount;     //!< Delayed ACK counter
  uint32_t          m_delAckMaxCount;  //!< Number of packet to fire an ACK before delay timeout
  bool              m_noDelay;         //!< Set to true to disable Nagle's algorithm
  uint32_t          m_synCount;        //!< Count of remaining connection retries
  uint32_t          m_synRetries;      //!< Number of connection attempts
  uint32_t          m_dataRetrCount;   //!< Count of remaining data retransmission attempts
  uint32_t          m_dataRetries;     //!< Number of data retransmission attempts
  Time              m_minRto;          //!< minimum value of the Retransmit timeout
  Time              m_clockGranularity; //!< Clock Granularity used in RTO calcs
  Time              m_delAckTimeout;   //!< Time to delay an ACK
  Time              m_persistTimeout;  //!< Time between sending 1-byte probes
  Time              m_cnTimeout;       //!< Timeout for connection retry
  RttHistory_t      m_history;         //!< List of sent packet

  // Connections to other layers of TCP/IP
  Ipv4EndPoint*       m_endPoint;   //!< the IPv4 endpoint
  Ipv6EndPoint*       m_endPoint6;  //!< the IPv6 endpoint
  Ptr<Node>           m_node;       //!< the associated node
  Ptr<TcpL4Protocol>  m_tcp;        //!< the associated TCP L4 protocol

  Ptr<RttEstimator> m_rtt; //!< Round trip time estimator

  // Rx and Tx buffer management
  Ptr<TcpRxBuffer>              m_rxBuffer;       //!< Rx buffer (reordering buffer)
  Ptr<TcpTxBuffer>              m_txBuffer;       //!< Tx buffer

  // State-related attributes
  TracedValue<TcpSocket::TcpStates_t> m_state; //!< TCP state
  mutable enum Socket::SocketErrno m_errno; //!< Socket error code
  bool                     m_closeNotified; //!< Told app to close socket
  bool                     m_closeOnEmpty;  //!< Close socket upon tx buffer emptied
  bool                     m_shutdownSend;  //!< Send no longer allowed
  bool                     m_shutdownRecv;  //!< Receive no longer allowed
  bool                     m_connected;     //!< Connection established
  double                   m_msl;           //!< Max segment lifetime

  // Window management
  uint16_t                      m_maxWinSize;     //!< Maximum window size to advertise
  SequenceNumber32              m_highTxAck;      //!< Highest ack sent
  uint32_t                      m_bytesAckedNotProcessed;  //!< Bytes acked, but not processed

  // Options
  bool    m_sackEnabled;       //!< RFC SACK option enabled
  bool    m_winScalingEnabled; //!< Window Scale option enabled (RFC 7323)
  uint8_t m_rcvWindShift;      //!< Window shift to apply to outgoing segments
  uint8_t m_sndWindShift;      //!< Window shift to apply to incoming segments

  bool     m_timestampEnabled;    //!< Timestamp option enabled
  uint32_t m_timestampToEcho;     //!< Timestamp to echo

  EventId m_sendPendingDataEvent; //!< micro-delay event to send pending data

  // Fast Retransmit and Recovery
  SequenceNumber32       m_recover;      //!< Previous highest Tx seqnum for fast recovery
  uint32_t               m_retxThresh;   //!< Fast Retransmit threshold
  bool                   m_limitedTx;    //!< perform limited transmit

  // Transmission Control Block
  Ptr<TcpSocketState>    m_tcb;               //!< Congestion control informations
  Ptr<TcpCongestionOps>  m_congestionControl; //!< Congestion control

  // Guesses over the other connection end
  bool m_isFirstPartialAck; //!< First partial ACK during RECOVERY
};

} // namespace ns3

#endif /* NS3_TCP_IMPLEMENTATION_H */
