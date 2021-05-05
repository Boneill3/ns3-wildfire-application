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
#include "ns3/mobility-module.h"

#include "ns3/wildfire-module.h"

//        Network Topology
//                     10.1.2.0
//                   *          *
//       10.1.1.0    |          |
// n0 -------------- n1         n2
//    point-to-point   ad-hoc Wifi
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wildfire example");

int
main (int argc, char *argv[])
{
  uint32_t nPackets = 1;

  CommandLine cmd (__FILE__);
  cmd.AddValue("nPackets", "Number of packets to echo", nPackets);
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS);
  LogComponentEnable ("WildfireClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("WildfireServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Creating Topology");

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  // Wifi Related
  NodeContainer wifiNodes;
  wifiNodes.Create (1);

  NodeContainer wifiBroadcaster = p2pNodes.Get(0);

  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211ac);
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

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiBroadcaster);
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (wifiNodes);
  mobility.Install (wifiBroadcaster);

  InternetStackHelper stack;
  stack.Install (wifiNodes);
  stack.Install (wifiBroadcaster); 
  stack.Install (p2pNodes.Get(1));
  //End wifi related

  PointToPointHelper pointToPoint;

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (p2pDevices);

  // Wifi Network
  address.SetBase ("10.1.2.0", "255.255.255.0");
  address.Assign(wifiDevices);
  Ipv4InterfaceContainer wifiInterfaces = address.Assign(apDevices);

  WildfireServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  echoServer.ScheduleNotification(serverApps.Get(0), Seconds(5.0));

  WildfireClientHelper echoClient (interfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (p2pNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  echoClient.SetFill(clientApps.Get(0), "Subscribe");

  WildfireClientHelper echoClient2 (wifiInterfaces.GetAddress (0), 49153);
  echoClient2.SetAttribute ("MaxPackets", UintegerValue (nPackets));
  echoClient2.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient2.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps2 = echoClient2.Install (wifiNodes.Get (0));
  clientApps2.Start (Seconds (3.0));
  clientApps2.Stop (Seconds (10.0));
  echoClient2.SetFill(clientApps2.Get(0), "WIFIMESSAGE");

  // This allows for global routing across connection types
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
