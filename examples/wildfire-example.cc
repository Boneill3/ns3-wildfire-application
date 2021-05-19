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
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/csma-module.h"
#include "ns3/csma-layout-module.h"
#include "ns3/mobility-module.h"

#include "ns3/wildfire-module.h"

//        Network Topology
// csma star layout internet connection, disrupted at 5s
// adhoc wifi to allow for transmission between devices
// when "internet" is unavailable

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wildfire example");

void disconnect (Ptr<NetDevice> router);

int
main (int argc, char *argv[])
{
  uint32_t nPackets = 1;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nPackets", "Number of packets to echo", nPackets);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("WildfireClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("WildfireServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Creating Topology");

  // Connected Internet state
  CsmaHelper csma;
  CsmaStarHelper star (3, csma);
  InternetStackHelper internet;
  star.InstallStack (internet);
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

  // Wifi Related
  NodeContainer wifiNodes;
  wifiNodes.Add (star.GetSpokeNode (1));
  wifiNodes.Add (star.GetSpokeNode (2));

  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211ac);
  WifiMacHelper wifiMac;
  YansWifiPhyHelper wifiPhy;
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiMac.SetType ("ns3::AdhocWifiMac");

  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("OfdmRate54Mbps"));

  YansWifiPhyHelper phy = wifiPhy;
  phy.SetChannel (wifiChannel.Create ());

  WifiMacHelper mac = wifiMac;
  NetDeviceContainer wifiDevices = wifi.Install (phy, mac, wifiNodes);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifiNodes);
  //End wifi related

  Ipv4AddressHelper address;

  // Wifi Network
  address.SetBase ("10.2.1.0", "255.255.255.0");
  address.Assign (wifiDevices);

  WildfireServerHelper echoServer (202);

  ApplicationContainer serverApps = echoServer.Install (star.GetSpokeNode (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  echoServer.ScheduleNotification (serverApps.Get (0), Seconds (5.0));

  WildfireClientHelper echoClient (star.GetSpokeIpv4Address (0), 202);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (star.GetSpokeNode (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  echoClient.ScheduleSubscription (clientApps.Get (0), Seconds (2.5), star.GetSpokeIpv4Address (0));

  WildfireClientHelper echoClient2 (star.GetSpokeIpv4Address (0), 202);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps2 = echoClient2.Install (star.GetSpokeNode (2));
  clientApps2.Start (Seconds (3.0));
  clientApps2.Stop (Seconds (10.0));
  echoClient2.ScheduleSubscription (clientApps2.Get (0), Seconds (3.5), star.GetSpokeIpv4Address (0));

  // This allows for global routing across connection types
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));

  // Schedule Network Disruption
  // Server (Internet)
  //Simulator::Schedule (Seconds (4.0), disconnect, star.GetHub ()->GetDevice (0));
  // Wireless Device 1
  //Simulator::Schedule (Seconds (4.0), disconnect, star.GetHub ()->GetDevice (1));
  // Wireless Device 2
  Simulator::Schedule (Seconds (4.0), disconnect, star.GetHub ()->GetDevice (2));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

void disconnect (Ptr<NetDevice> router)
{
  auto csmaRouter = DynamicCast<CsmaNetDevice, NetDevice> (router);
  csmaRouter->SetSendEnable (false);
  NS_LOG_INFO ("Network Disconnected");
}
