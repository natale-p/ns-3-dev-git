/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "ns3/test.h"
#include "ns3/tcp-tx-buffer.h"
#include "ns3/packet.h"
#include "ns3/log.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpTxBufferTestSuite");

class TcpTxBufferTestCase : public TestCase
{
public:
  TcpTxBufferTestCase ();

private:
  virtual void DoRun (void);
  virtual void DoTeardown (void);

  void TestNewBlock ();
  void TestTransmittedBlock ();
  void TestUpdateScoreboardWithCraftedSACK ();
};

TcpTxBufferTestCase::TcpTxBufferTestCase ()
  : TestCase ("TcpTxBuffer Test")
{
}

void
TcpTxBufferTestCase::DoRun ()
{
  /*
   * Cases for new block:
   * -> is exactly the same as stored
   * -> starts over the boundary, but ends earlier
   * -> starts over the boundary, but ends after
   */
  TestNewBlock ();

  /*
   * Cases for transmitted block:
   * -> is exactly the same as previous
   * -> starts over the boundary, but ends earlier
   * -> starts over the boundary, but ends after
   * -> starts inside a packet, ends right
   * -> starts inside a packet, ends earlier in the same packet
   * -> starts inside a packet, ends in another packet
   */
  TestTransmittedBlock ();

  TestUpdateScoreboardWithCraftedSACK ();
}

void
TcpTxBufferTestCase::TestUpdateScoreboardWithCraftedSACK ()
{
  TcpTxBuffer txBuf;
  SequenceNumber32 head (1);
  txBuf.SetHeadSequence (head);

  // Add a single, 3000-bytes long, packet
  txBuf.Add (Create<Packet> (30000));

  // Simulate sending 100 packets, 150 bytes long each, from seq 1
  for (uint32_t i=0; i<100; ++i)
    {
      txBuf.CopyFromSequence (150, head + (150 * i));
    }

  // Now we have 100 packets sent, 100 waiting (well, that 100 are condensed in one)

  // Suppose now we receive 99 dupacks, because the first was lost.
  for (uint32_t i=0; i<99; ++i)
    {
      Ptr<const TcpOptionSack> sack = txBuf.CraftSackOption (head, 32); // 3 maximum sack block

      // For iteration 0 and 1 we have 1 and 2 sack blocks, respectively.
      // For all others, maximum 3
      if (i == 0)
        {
          NS_TEST_ASSERT_MSG_EQ (sack->GetNumSackBlocks (), 1,
                                 "Different block number than expected");
        }
      else if (i == 1)
        {
          NS_TEST_ASSERT_MSG_EQ (sack->GetNumSackBlocks (), 2,
                                 "Different block number than expected");
        }
      else if (i >= 2)
        {
          NS_TEST_ASSERT_MSG_EQ (sack->GetNumSackBlocks (), 3,
                                 "More blocks than expected");
        }

      TcpOptionSack::SackList sackList = sack->GetSackList ();
      TcpOptionSack::SackBlock block = sackList.front ();
      sackList.pop_front();

      // The first block, assuming all the other are SACKed in order (from 2nd
      // onward) has seq = 1 + (150 * (i+1)) --> i+1 because the first sent
      // block cannot be SACKed
      NS_TEST_ASSERT_MSG_EQ (block.first, SequenceNumber32 (1 + (150*(i+1))),
                             "First sack block is wrong (on the left)");
      NS_TEST_ASSERT_MSG_EQ (block.second, block.first + 150,
                             "First sack block is wrong (on the right)");

      SequenceNumber32 left = block.first;
      for (TcpOptionSack::SackList::iterator it = sackList.begin(); it != sackList.end(); ++it)
        {
          block = (*it);

          // the blocks go backwards here. To understand better, an example
          // of SACK option list: [1351;1501], [1201;1351], [1051;1201]
          NS_TEST_ASSERT_MSG_EQ (block.first, left - 150,
                                 "First sack block is wrong (on the left)");
          NS_TEST_ASSERT_MSG_EQ (block.second, left,
                                 "First sack block is wrong (on the right)");
          left -= 150;
        }

      txBuf.Update (sack->GetSackList());
    }
}

void
TcpTxBufferTestCase::TestNewBlock ()
{
  // Manually recreating all the conditions
  TcpTxBuffer txBuf;
  txBuf.SetHeadSequence (SequenceNumber32 (1));

  // get a packet which is exactly the same stored
  Ptr<Packet> p1 = Create<Packet> (100);
  txBuf.Add (p1);

  NS_TEST_ASSERT_MSG_EQ (txBuf.SizeFromSequence (SequenceNumber32 (1)), 100,
                         "TxBuf miscalculates size");
  //NS_TEST_ASSERT_MSG_EQ (txBuf.BytesInFlight (), 0,
  //                       "TxBuf miscalculates size of in flight segments");

  Ptr<Packet> ret = txBuf.CopyFromSequence (100, SequenceNumber32 (1));
  NS_TEST_ASSERT_MSG_EQ (ret->GetSize (), 100,
                         "Returned packet has different size than requested");
  NS_TEST_ASSERT_MSG_EQ (txBuf.SizeFromSequence (SequenceNumber32 (1)), 100,
                         "TxBuf miscalculates size");
  //NS_TEST_ASSERT_MSG_EQ (txBuf.BytesInFlight (), 100,
  //                       "TxBuf miscalculates size of in flight segments");

  txBuf.DiscardUpTo (SequenceNumber32 (101));
  NS_TEST_ASSERT_MSG_EQ (txBuf.SizeFromSequence (SequenceNumber32 (101)), 0,
                         "TxBuf miscalculates size");
  //NS_TEST_ASSERT_MSG_EQ (txBuf.BytesInFlight (), 0,
  //                       "TxBuf miscalculates size of in flight segments");

  // starts over the boundary, but ends earlier

  Ptr<Packet> p2 = Create<Packet> (100);
  txBuf.Add (p2);

  ret = txBuf.CopyFromSequence (50, SequenceNumber32 (101));
  NS_TEST_ASSERT_MSG_EQ (ret->GetSize (), 50,
                         "Returned packet has different size than requested");
  NS_TEST_ASSERT_MSG_EQ (txBuf.SizeFromSequence (SequenceNumber32 (151)), 50,
                         "TxBuf miscalculates size");
  //NS_TEST_ASSERT_MSG_EQ (txBuf.BytesInFlight (), 50,
  //                       "TxBuf miscalculates size of in flight segments");

  // starts over the boundary, but ends after
  Ptr<Packet> p3 = Create<Packet> (100);
  txBuf.Add (p3);

  ret = txBuf.CopyFromSequence (70, SequenceNumber32 (151));
  NS_TEST_ASSERT_MSG_EQ (ret->GetSize (), 70,
                         "Returned packet has different size than requested");
  NS_TEST_ASSERT_MSG_EQ (txBuf.SizeFromSequence (SequenceNumber32 (221)), 80,
                         "TxBuf miscalculates size");
  //NS_TEST_ASSERT_MSG_EQ (txBuf.BytesInFlight (), 120,
  //                       "TxBuf miscalculates size of in flight segments");

  ret = txBuf.CopyFromSequence (3000, SequenceNumber32 (221));
  NS_TEST_ASSERT_MSG_EQ (ret->GetSize (), 80,
                         "Returned packet has different size than requested");
  NS_TEST_ASSERT_MSG_EQ (txBuf.SizeFromSequence (SequenceNumber32 (301)), 0,
                         "TxBuf miscalculates size");
  //NS_TEST_ASSERT_MSG_EQ (txBuf.BytesInFlight (), 200,
  //                       "TxBuf miscalculates size of in flight segments");

  // Clear everything
  txBuf.DiscardUpTo (SequenceNumber32 (381));
  NS_TEST_ASSERT_MSG_EQ (txBuf.Size (), 0,
                         "Size is different than expected");
}

void
TcpTxBufferTestCase::TestTransmittedBlock ()
{
}

void
TcpTxBufferTestCase::DoTeardown ()
{
}

class TcpTxBufferTestSuite : public TestSuite
{
public:
  TcpTxBufferTestSuite ()
    : TestSuite ("tcp-tx-buffer", UNIT)
  {
    AddTestCase (new TcpTxBufferTestCase, TestCase::QUICK);
  }
};
static TcpTxBufferTestSuite  g_tcpTxBufferTestSuite;
