/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 25Dic-1Apr 2018 {natale.patriciello,pasquale.imputato}@gmail.com
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
#include "packet-socket-tc.h"
#include <ns3/traffic-control-layer.h>
#include <ns3/ipv4-queue-disc-item.h>
#include <ns3/ipv4-l3-protocol.h>
#include <ns3/log.h>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PacketSocketTc");

NS_OBJECT_ENSURE_REGISTERED (PacketSocketTc);

TypeId
PacketSocketTc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::PacketSocketTc")
    .SetParent<PacketSocket> ()
    .AddConstructor<PacketSocketTc> ()
    .SetGroupName("Lte")
  ;
  return tid;
}

PacketSocketTc::PacketSocketTc() : PacketSocket()
{

}

void PacketSocketTc::SetNode(Ptr<Node> node)
{
  PacketSocket::SetNode(node);

  m_tc = node->GetObject<TrafficControlLayer> ();
  NS_ABORT_IF (m_tc == nullptr);
}

bool PacketSocketTc::DoSend(Ptr<NetDevice> dev, Ptr<Packet> packet,
                            const Address &dest, uint16_t protocolNumber)
{
  NS_ABORT_IF(protocolNumber != Ipv4L3Protocol::PROT_NUMBER);

  Ipv4Header ipv4Header;
  packet->RemoveHeader(ipv4Header);

  Ptr<QueueDiscItem> item;
  item = Create<Ipv4QueueDiscItem> (packet, dest, protocolNumber, ipv4Header);

  m_tc->Send(dev, item);
  return true;
}

} // namespace ns3
