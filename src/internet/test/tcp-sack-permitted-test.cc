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

#include "tcp-general-test.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/tcp-option-sack-permitted.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SACKPermittedTestSuite");

class SACKPermittedTestCase : public TcpGeneralTest
{
public:
  enum Configuration
  {
    DISABLED,
    ENABLED_RECEIVER,
    ENABLED_SENDER,
    ENABLED
  };

  SACKPermittedTestCase (SACKPermittedTestCase::Configuration conf);
protected:
  virtual Ptr<TcpSocketMsgBase> CreateReceiverSocket (Ptr<Node> node);
  virtual Ptr<TcpSocketMsgBase> CreateSenderSocket (Ptr<Node> node);

  virtual void Tx (const Ptr<const Packet> p, const TcpHeader&h, SocketWho who);

  Configuration m_configuration;
};

SACKPermittedTestCase::SACKPermittedTestCase (SACKPermittedTestCase::Configuration conf)
  : TcpGeneralTest ("Testing the TCP SACK Permitted option")
{
  m_configuration = conf;
}

Ptr<TcpSocketMsgBase>
SACKPermittedTestCase::CreateReceiverSocket (Ptr<Node> node)
{
  Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateReceiverSocket (node);

  switch (m_configuration)
    {
    case DISABLED:
      socket->SetAttribute ("SACK", BooleanValue (false));
      break;

    case ENABLED_RECEIVER:
      socket->SetAttribute ("SACK", BooleanValue (true));
      break;

    case ENABLED_SENDER:
      socket->SetAttribute ("SACK", BooleanValue (false));
      break;

    case ENABLED:
      socket->SetAttribute ("SACK", BooleanValue (true));
      break;
    }

  return socket;
}

Ptr<TcpSocketMsgBase>
SACKPermittedTestCase::CreateSenderSocket (Ptr<Node> node)
{
  Ptr<TcpSocketMsgBase> socket = TcpGeneralTest::CreateSenderSocket (node);

  switch (m_configuration)
    {
    case DISABLED:
      socket->SetAttribute ("SACK", BooleanValue (false));
      break;

    case ENABLED_RECEIVER:
      socket->SetAttribute ("SACK", BooleanValue (false));
      break;

    case ENABLED_SENDER:
      socket->SetAttribute ("SACK", BooleanValue (true));
      break;

    case ENABLED:
      socket->SetAttribute ("SACK", BooleanValue (true));
      break;
    }

  return socket;
}

void
SACKPermittedTestCase::Tx (const Ptr<const Packet> p, const TcpHeader &h, SocketWho who)
{

  if (!(h.GetFlags () & TcpHeader::SYN))
    {
      NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                             "SACKPermitted in non-SYN segment");
      return;
    }

  if (m_configuration == DISABLED)
    {
      NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                             "SACKPermitted disabled but option enabled");
    }
  else if (m_configuration == ENABLED)
    {
      NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), true,
                             "SACKPermitted enabled but option disabled");
    }

  NS_LOG_INFO (h);
  if (who == SENDER)
    {
      if (h.GetFlags () & TcpHeader::SYN)
        {
          if (m_configuration == ENABLED_RECEIVER)
            {
              NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                                     "SACKPermitted disabled but option enabled");
            }
          else if (m_configuration == ENABLED_SENDER)
            {
              NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), true,
                                     "SACKPermitted enabled but option disabled");
            }
        }
      else
        {
          if (m_configuration != ENABLED)
            {
              NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                                     "SACKPermitted disabled but option enabled");
            }
        }
    }
  else if (who == RECEIVER)
    {
      if (h.GetFlags () & TcpHeader::SYN)
        {
          // Sender has not sent SACKPermitted, so implementation should disable ts
          if (m_configuration == ENABLED_RECEIVER)
            {
              NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                                     "sender has not ts, but receiver sent anyway");
            }
          else if (m_configuration == ENABLED_SENDER)
            {
              NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                                     "receiver has not ts enabled but sent anyway");
            }
        }
      else
        {
          if (m_configuration != ENABLED)
            {
              NS_TEST_ASSERT_MSG_EQ (h.HasOption (TcpOption::SACKPERMITTED), false,
                                     "SACKPermitted disabled but option enabled");
            }
        }
    }
}

static class TcpSACKPermittedTestSuite : public TestSuite
{
public:
  TcpSACKPermittedTestSuite ()
    : TestSuite ("tcp-sack-permitted", UNIT)
  {
    AddTestCase (new SACKPermittedTestCase (SACKPermittedTestCase::DISABLED), TestCase::QUICK);
    AddTestCase (new SACKPermittedTestCase (SACKPermittedTestCase::ENABLED_RECEIVER), TestCase::QUICK);
    AddTestCase (new SACKPermittedTestCase (SACKPermittedTestCase::ENABLED_SENDER), TestCase::QUICK);
    AddTestCase (new SACKPermittedTestCase (SACKPermittedTestCase::ENABLED), TestCase::QUICK);
  }

} g_tcpSACKPermittedTestSuite;

} // namespace ns3
