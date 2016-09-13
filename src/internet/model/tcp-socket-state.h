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
#ifndef NS3_TCP_SOCKET_STATE_H
#define NS3_TCP_SOCKET_STATE_H

#include "ns3/object.h"
#include "ns3/sequence-number.h"
#include "ns3/traced-value.h"

namespace ns3 {

class StateTracedValues
{
public:
  StateTracedValues ();
  StateTracedValues (const StateTracedValues &other);
  ~StateTracedValues ();

  TracedValue<uint32_t>  *m_cWnd;            //!< Congestion window
  TracedValue<uint32_t>  *m_ssThresh;        //!< Slow start threshold

  TracedValue<SequenceNumber32> *m_highTxMark; //!< Highest seqno ever sent, regardless of ReTx
  TracedValue<SequenceNumber32> *m_nextTxSequence; //!< Next seqnum to be sent (SND.NXT), ReTx pushes it back
};

/**
 * \brief Data structure that records the congestion state of a connection
 *
 * In this data structure, basic informations that should be passed between
 * socket and the congestion control algorithm are saved. Through the code,
 * it will be referred as Transmission Control Block (TCB), but there are some
 * differencies. In the RFCs, the TCB contains all the variables that defines
 * a connection, while we preferred to maintain in this class only the values
 * that should be exchanged between socket and other parts, like congestion
 * control algorithms.
 *
 */
class TcpSocketState : public Object
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  TcpSocketState ();

  /**
   * \brief Copy constructor.
   * \param other object to copy.
   */
  TcpSocketState (const TcpSocketState &other);

  /**
   * \brief Definition of the Congestion state machine
   *
   * The design of this state machine is taken from Linux v4.0, but it has been
   * maintained in the Linux mainline from ages. It basically avoids to maintain
   * a lot of boolean variables, and it allows to check the transitions from
   * different algorithm in a cleaner way.
   *
   * These states represent the situation from a congestion control point of view:
   * in fact, apart the CA_OPEN state, the other states represent a situation in
   * which there is a congestion, and different actions should be taken,
   * depending on the case.
   *
   */
  typedef enum
  {
    CA_OPEN,      /**< Normal state, no dubious events */
    CA_DISORDER,  /**< In all the respects it is "Open",
                    *  but requires a bit more attention. It is entered when
                    *  we see some SACKs or dupacks. It is split of "Open" */
    CA_CWR,       /**< cWnd was reduced due to some Congestion Notification event.
                    *  It can be ECN, ICMP source quench, local device congestion.
                    *  Not used in NS-3 right now. */
    CA_RECOVERY,  /**< CWND was reduced, we are fast-retransmitting. */
    CA_LOSS,      /**< CWND was reduced due to RTO timeout or SACK reneging. */
    CA_LAST_STATE /**< Used only in debug messages */
  } TcpCongState_t;

  /**
   * \ingroup tcp
   * TracedValue Callback signature for TcpCongState_t
   *
   * \param [in] oldValue original value of the traced variable
   * \param [in] newValue new value of the traced variable
   */
  typedef void (* TcpCongStatesTracedValueCallback)(const TcpCongState_t oldValue,
                                                    const TcpCongState_t newValue);

  /**
   * \brief Literal names of TCP states for use in log messages
   */
  static const char* const TcpCongStateName[TcpSocketState::CA_LAST_STATE];

  // Congestion control
  uint32_t               m_initialCWnd;     //!< Initial cWnd value
  uint32_t               m_initialSsThresh; //!< Initial Slow Start Threshold value

  // Segment
  uint32_t               m_segmentSize;     //!< Segment size
  SequenceNumber32       m_lastAckedSeq;    //!< Last sequence ACKed

  TracedValue<TcpCongState_t> m_congState;    //!< State in the Congestion state machine

  /**
   * \brief Get cwnd in segments rather than bytes
   *
   * \return Congestion window in segments
   */
  uint32_t GetCwndInSegments () const
  {
    return GetCwnd () / m_segmentSize;
  }

  /**
   * \brief Get slow start thresh in segments rather than bytes
   *
   * \return Slow start threshold in segments
   */
  uint32_t GetSsThreshInSegments () const
  {
    return GetSsThresh () / m_segmentSize;
  }

  uint32_t GetSsThresh () const
  {
    return m_tracedValues.m_ssThresh->Get ();
  }

  uint32_t GetCwnd () const
  {
    return m_tracedValues.m_cWnd->Get ();
  }

  SequenceNumber32 GetNextTxSequence () const
  {
    return m_tracedValues.m_nextTxSequence->Get ();
  }

  void SetCwnd (uint32_t cWnd)
  {
    *m_tracedValues.m_cWnd = cWnd;
  }

  void SetSsThresh (uint32_t ssThresh)
  {
    *m_tracedValues.m_ssThresh = ssThresh;
  }

  void SetTracedValues (const StateTracedValues &tracedValues)
  {
    m_tracedValues = tracedValues;
  }

private:
  StateTracedValues m_tracedValues;
};

} // namespace ns3
#endif // NS3_TCP_SOCKET_STATE_H
