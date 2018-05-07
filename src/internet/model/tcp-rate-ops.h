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

#include "ns3/object.h"
#include "ns3/tcp-tx-item.h"
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"

namespace ns3 {

/**
 * \brief Interface for all operations that involve a Rate monitoring for TCP.
 */
class TcpRateOps : public Object
{
public:
  struct TcpRateSample;

  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief Put the rate information inside the sent skb
   *
   * Snapshot the current delivery information in the skb, to generate
   * a rate sample later when the skb is (s)acked in SkbDelivered ().
   *
   * \param skb The SKB sent
   * \param isStartOfTransmission true if this is a start of transmission
   * (i.e., in_flight == 0)
   */
  virtual void SkbSent (TcpTxItem *skb, bool isStartOfTransmission) = 0;

  /**
   * \brief Update the Rate information after an item is received
   *
   * When an skb is sacked or acked, we fill in the rate sample with the (prior)
   * delivery information when the skb was last transmitted.
   *
   * If an ACK (s)acks multiple skbs (e.g., stretched-acks), this function is
   * called multiple times. We favor the information from the most recently
   * sent skb, i.e., the skb with the highest prior_delivered count.
   *
   * \param skb The SKB delivered ((s)ACKed)
   */
  virtual void SkbDelivered (TcpTxItem * skb) = 0;

  /**
   * \brief If a gap is detected between sends, it means we are app-limited.
   * \return TODO What the Linux kernel is setting in tp->app_limited?
   * https://elixir.bootlin.com/linux/latest/source/net/ipv4/tcp_rate.c#L177
   *
   * \param cWnd Congestion Window
   * \param in_flight In Flight size (in bytes)
   * \param segmentSize Segment size
   * \param tailSeq Tail Sequence
   * \param nextTx NextTx
   */
  virtual void CalculateAppLimited (uint32_t cWnd, uint32_t in_flight,
                                    uint32_t segmentSize, const SequenceNumber32 &tailSeq,
                                    const SequenceNumber32 &nextTx) = 0;

  /**
   *
   * \brief Generate a TcpRateSample to feed a congestion avoidance algorithm
   *
   * \brief RateGen
   * \param delivered
   * \param lost
   * \param is_sack_reneg
   * \param minRtt
   * \return
   */
  virtual const TcpRateSample & SampleGen (uint32_t delivered, uint32_t lost,
                                           bool is_sack_reneg,
                                           const Time &minRtt) = 0;

  /**
   * \brief Rate Sample structure
   *
   * A rate sample measures the number of (original/retransmitted) data
   * packets delivered "delivered" over an interval of time "interval_us".
   * The tcp_rate code fills in the rate sample, and congestion
   * control modules that define a cong_control function to run at the end
   * of ACK processing can optionally chose to consult this sample when
   * setting cwnd and pacing rate.
   * A sample is invalid if "delivered" or "interval_us" is negative.
   */
  struct TcpRateSample
  {
    DataRate      m_deliveryRate   {DataRate ("0bps")};//!< The delivery rate sample
    uint32_t      m_isAppLimited   {0};                //!< Indicates whether the rate sample is application-limited
    Time          m_interval       {Seconds (0.0)};    //!< The length of the sampling interval
    uint32_t      m_delivered      {0};                //!< The amount of data marked as delivered over the sampling interval
    uint32_t      m_priorDelivered {0};                //!< The delivered count of the most recent packet delivered
    Time          m_priorTime      {Seconds (0.0)};    //!< The delivered time of the most recent packet delivered
    Time          m_sendElapsed    {Seconds (0.0)};    //!< Send time interval calculated from the most recent packet delivered
    Time          m_ackElapsed     {Seconds (0.0)};    //!< ACK time interval calculated from the most recent packet delivered
    uint32_t      m_packetLoss     {0};
    uint32_t      m_priorInFlight  {0};

    /**
     * \brief Is the sample valid?
     * \return true if the sample is valid, false otherwise.
     */
    bool IsValid () const
    {
      return (m_priorTime != Seconds (0.0) || m_interval != Seconds (0.0));
    }
  };
};

/**
 * \brief Linux management and generation of Rate information for TCP
 */
class TcpRateLinux : public TcpRateOps
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual ~TcpRateLinux () override {}

  virtual void SkbSent (TcpTxItem *skb, bool isStartOfTransmission) override;
  virtual void SkbDelivered (TcpTxItem * skb) override;
  virtual void CalculateAppLimited (uint32_t cWnd, uint32_t in_flight,
                                  uint32_t segmentSize, const SequenceNumber32 &tailSeq,
                                  const SequenceNumber32 &nextTx) override;
  virtual const TcpRateSample & SampleGen (uint32_t delivered, uint32_t lost,
                                           bool is_sack_reneg,
                                           const Time &minRtt) override;

private:
  /**
   * \brief The TcpRate struct
   */
  struct TcpRate
  {
    uint64_t               m_delivered       {0};           //!< The total amount of data in bytes delivered so far
    Time                   m_deliveredTime   {Seconds (0)}; //!< Simulator time when m_delivered was last updated
    Time                   m_firstSentTime   {Seconds (0)}; //!< The send time of the packet that was most recently marked as delivered
    uint32_t               m_appLimited      {0};           //!< The index of the last transmitted packet marked as application-limited
    uint32_t               m_txItemDelivered {0};           //!< TODO Adding DOC
    uint32_t               m_lastAckedSackedBytes {0};      //!< Size of data sacked in the last ack
  };

  // Rate sample related variables
  TcpRate       m_rate;         //!< Rate information
  TcpRateSample m_rateSample;   //!< Rate sample (continuosly updated)

  TracedCallback<const TcpRate &>       m_rateTrace;       //!< Rate trace  (TODO)
  TracedCallback<const TcpRateSample &> m_rateSampleTrace; //!< Rate Sample trace (TODO)
};

} //namespace ns3
