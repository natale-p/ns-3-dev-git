/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
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
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  http://wiki.ittc.ku.edu/resilinets
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#include "ns3/test.h"
#include "ns3/log.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-veno.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpVenoTestSuite");

static uint32_t
NewReno_SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  if (segmentsAcked >= 1)
    {
      tcb->SetCwnd (tcb->GetCwnd () + tcb->m_segmentSize);
      return segmentsAcked - 1;
    }

  return 0;
}

static void
NewReno_CongestionAvoidance (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  if (segmentsAcked > 0)
    {
      double adder = static_cast<double> (tcb->m_segmentSize * tcb->m_segmentSize) / tcb->GetCwnd ();
      adder = std::max (1.0, adder);
      tcb->SetCwnd (tcb->GetCwnd () + static_cast<uint32_t> (adder));
    }
}

static void
NewReno_IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  if (tcb->GetCwnd () < tcb->GetSsThresh ())
    {
      segmentsAcked = NewReno_SlowStart (tcb, segmentsAcked);
    }

  if (tcb->GetCwnd () >= tcb->GetSsThresh ())
    {
      NewReno_CongestionAvoidance (tcb, segmentsAcked);
    }
}

/**
 * \brief Testing the additive increase and multiplicative decrease of TcpVeno
 */
class TcpVenoTest : public TestCase
{
public:
  TcpVenoTest (uint32_t cWnd,
               uint32_t segmentSize,
               uint32_t ssThresh,
               Time rtt,
               uint32_t segmentsAcked,
               uint32_t numRtt,
               const std::string &name);

private:
  virtual void DoRun (void);
  void AdditiveIncrease (Ptr<TcpSocketState> state, uint32_t diff, UintegerValue beta);
  uint32_t MultiplicativeDecrease (uint32_t diff, const UintegerValue &beta, uint32_t bytesInFlight);

  uint32_t m_cWnd;
  uint32_t m_segmentSize;
  uint32_t m_ssThresh;
  Time m_rtt;
  uint32_t m_segmentsAcked;
  uint32_t m_numRtt;
  bool m_inc;
  Ptr<TcpSocketState> m_state;
};

TcpVenoTest::TcpVenoTest (uint32_t cWnd,
                          uint32_t segmentSize,
                          uint32_t ssThresh,
                          Time rtt,
                          uint32_t segmentsAcked,
                          uint32_t numRtt,
                          const std::string &name)
  : TestCase (name),
    m_cWnd (cWnd),
    m_segmentSize (segmentSize),
    m_ssThresh (ssThresh),
    m_rtt (rtt),
    m_segmentsAcked (segmentsAcked),
    m_numRtt (numRtt),
    m_inc (true)
{
}

void
TcpVenoTest::DoRun ()
{
  TracedValue<uint32_t> localCwnd = m_cWnd;
  TracedValue<uint32_t> localSsThresh = m_ssThresh;
  StateTracedValues tracedValues;
  tracedValues.m_cWnd = &localCwnd;
  tracedValues.m_ssThresh = &localSsThresh;

  m_state = CreateObject <TcpSocketState> ();
  m_state->SetTracedValues (tracedValues);
  m_state->m_segmentSize = m_segmentSize;

  Ptr<TcpVeno> cong = CreateObject <TcpVeno> ();

  // Set baseRtt to 100 ms
  Time baseRtt = MilliSeconds (100);
  cong->PktsAcked (m_state, m_segmentsAcked, baseRtt);

  // Re-set Veno to assign a new value of minRtt
  cong->CongestionStateSet (m_state, TcpSocketState::CA_OPEN);

  uint32_t segCwnd = m_cWnd / m_segmentSize;

  // Calculate expected throughput
  uint32_t expectedCwnd;
  double tmp = baseRtt.GetSeconds () / m_rtt.GetSeconds ();
  expectedCwnd = segCwnd * tmp;

  // Calculate the difference between actual and expected throughput
  uint32_t diff;
  diff = segCwnd - expectedCwnd;

  // Get the beta attribute
  UintegerValue beta;
  cong->GetAttribute ("Beta", beta);

  uint32_t cntRtt = 0;

  TracedValue<uint32_t> otherLocalCwnd = m_cWnd;
  TracedValue<uint32_t> otherLlocalSsThresh = m_ssThresh;
  StateTracedValues tracedValues2;
  tracedValues2.m_cWnd = &otherLocalCwnd;
  tracedValues2.m_ssThresh = &otherLlocalSsThresh;

  TcpSocketState state;
  state.m_segmentSize = m_segmentSize;
  state.SetTracedValues(tracedValues2);

  while (m_numRtt != 0)
    {
      // Update cwnd using Veno's additive increase algorithm
      cong->PktsAcked (m_state, m_segmentsAcked, m_rtt);
      cong->IncreaseWindow (m_state, m_segmentsAcked);

      // The first round the internal m_diff of cong will be 4, just like us
      if (cntRtt == 0)
        {
          // Update ssthresh using Veno's multiplicative decrease algorithm
          uint32_t ssThresh = cong->GetSsThresh (m_state, m_state->GetCwnd ());

          // Our calculation of ssthresh
          uint32_t calculatedSsThresh = MultiplicativeDecrease (diff, beta, m_state->GetCwnd ());

          NS_TEST_ASSERT_MSG_EQ (ssThresh, calculatedSsThresh,
                                 "Veno has not decremented cWnd correctly based on its"
                                 "multiplicative decrease algo.");
        }

      // Our calculation of cwnd
      if (cntRtt <= 2)
        {
          NewReno_IncreaseWindow (&state, 1);
        }
      else
        {
          AdditiveIncrease (&state, diff, beta);
        }

      NS_TEST_ASSERT_MSG_EQ (m_state->GetCwnd (), state.GetCwnd (),
                             "CWnd has not updated correctly based on Veno linear increase algorithm");
      m_numRtt--;
      cntRtt++;
    }
}

void
TcpVenoTest::AdditiveIncrease (Ptr<TcpSocketState> state, uint32_t diff, UintegerValue beta)
{
  if (m_cWnd < m_ssThresh)
    { // Slow start
      NewReno_SlowStart (state, 1);
    }
  else
    { // Congestion avoidance
      if (diff < beta.Get ())
        { // Increase cwnd by 1 every RTT when bandwidth is not fully utilized
          NewReno_CongestionAvoidance (state, 1);
        }
      else
        { // Increase cwnd by 1 every other RTT when bandwidth is fully utilized
          if (m_inc)
            {
              NewReno_CongestionAvoidance (state, 1);
              m_inc = false;
            }
          else
            {
              m_inc = true;
            }
        }
    }
}

uint32_t
TcpVenoTest::MultiplicativeDecrease (uint32_t diff, const UintegerValue &beta,
                                     uint32_t bytesInFlight)
{
  uint32_t calculatedSsThresh;
  if (diff < beta.Get ())
    {
      static double tmp = 4.0 / 5.0;
      calculatedSsThresh = std::max (2 * m_segmentSize, static_cast<uint32_t> (bytesInFlight * tmp));
    }
  else
    {
      calculatedSsThresh = std::max (2 * m_segmentSize, bytesInFlight / 2);
    }
  return calculatedSsThresh;
}


// -------------------------------------------------------------------
static class TcpVenoTestSuite : public TestSuite
{
public:
  TcpVenoTestSuite () : TestSuite ("tcp-veno-test", UNIT)
  {
    AddTestCase (new TcpVenoTest (38 * 1446, 1446, 40 * 1446, MilliSeconds (100), 1, 1,
                                  "Veno test on cWnd in slow start and non-congestive loss"),
                 TestCase::QUICK);
    AddTestCase (new TcpVenoTest (30 * 536, 536, 20 * 536, MilliSeconds (106), 1, 1,
                                  "Veno test on cWnd with diff < beta"),
                 TestCase::QUICK);
    AddTestCase (new TcpVenoTest (60 * 536, 536, 40 * 536, MilliSeconds (106), 1, 3,
                                  "Veno increment test on cWnd with diff > beta"),
                 TestCase::QUICK);
  }
} g_tcpVenoTest;

} // namespace ns3
