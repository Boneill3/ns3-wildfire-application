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

#include "ns3/basic-energy-source.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/energy-source-container.h"
#include "ns3/device-energy-model-container.h"

#include "ns3/wildfire-module.h"

//        Network Topology
// csma star layout internet connection, disrupted at 5s
// adhoc wifi to allow for transmission between devices
// when "internet" is unavailable

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wildfire example");

void disconnect (Ptr<NetDevice> router);

void RemainingEnergy (Ptr<OutputStreamWrapper> stream, double oldValue, double remainingEnergy);
void TotalEnergy (Ptr<OutputStreamWrapper> stream, double oldValue, double totalEnergy);
static void LogRecieved (Ptr<OutputStreamWrapper> stream);
static void LogSent (Ptr<OutputStreamWrapper> stream);
static void LogAck (Ptr<OutputStreamWrapper> stream);

uint64_t notifications_received = 0;
uint64_t total_sent_messages = 0;
uint64_t total_notification_acks = 0;
double total_power = 0;
uint64_t total_dead_battery = 0;

int
main (int argc, char *argv[])
{
  uint32_t nPackets = 1;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nPackets", "Number of packets to echo", nPackets);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  //LogComponentEnable ("WildfireClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("WildfireServerApplication", LOG_LEVEL_INFO);

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

  /** Energy Model **/
  /***************************************************************************/
  /* energy source */
  BasicEnergySourceHelper basicSourceHelper;
  // configure energy source
  basicSourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (100));
  // install source
  EnergySourceContainer sources = basicSourceHelper.Install (wifiNodes);
  /* device energy model */
  WifiRadioEnergyModelHelper radioEnergyHelper;
  // configure radio energy model
  radioEnergyHelper.Set ("TxCurrentA", DoubleValue (0.0174));
  radioEnergyHelper.Set ("RxCurrentA", DoubleValue (0.0197));
  // install device model
  DeviceEnergyModelContainer deviceModels = radioEnergyHelper.Install (wifiDevices, sources);
  /***************************************************************************/

  /** connect trace sources **/
  /***************************************************************************/
  // all sources are connected to node 1
  // energy source
  AsciiTraceHelper asciiTraceHelper2;
  Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream ("NoPowerCount.dat");

  for (int i = 0; i < sources.GetN (); ++i)
    {
      Ptr<BasicEnergySource> basicSourcePtr = DynamicCast<BasicEnergySource> (sources.Get (i));
      basicSourcePtr->TraceConnectWithoutContext ("RemainingEnergy", MakeBoundCallback (&RemainingEnergy, stream2));
      // device energy model
      Ptr<DeviceEnergyModel> basicRadioModelPtr =
        basicSourcePtr->FindDeviceEnergyModels ("ns3::WifiRadioEnergyModel").Get (0);
      NS_ASSERT (basicRadioModelPtr != NULL);
      Ptr<OutputStreamWrapper> stream2 = asciiTraceHelper2.CreateFileStream ("TotalPowerCount.dat");
      basicRadioModelPtr->TraceConnectWithoutContext ("TotalEnergyConsumption", MakeBoundCallback (&TotalEnergy, stream2));
    }
  /***************************************************************************/

  Ipv4AddressHelper address;

  // Wifi Network
  address.SetBase ("10.2.1.0", "255.255.255.0");
  address.Assign (wifiDevices);

  WildfireServerHelper echoServer (202);

  ApplicationContainer serverApps = echoServer.Install (star.GetSpokeNode (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  echoServer.ScheduleNotification (serverApps.Get (0), Seconds (5.0));

  WildfireClientHelper echoClient (star.GetSpokeIpv4Address (0), 202, 202);
  echoClient.SetAttribute ("BroadcastInterval", TimeValue (Seconds (2.0)));

  ApplicationContainer clientApps = echoClient.Install (star.GetSpokeNode (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));
  echoClient.ScheduleSubscription (clientApps.Get (0), Seconds (2.5), star.GetSpokeIpv4Address (0));

  WildfireClientHelper echoClient2 (star.GetSpokeIpv4Address (0), 202, 202);
  echoClient2.SetAttribute ("BroadcastInterval", TimeValue (Seconds (2.0)));

  ApplicationContainer clientApps2 = echoClient2.Install (star.GetSpokeNode (2));
  clientApps2.Start (Seconds (3.0));
  clientApps2.Stop (Seconds (10.0));
  echoClient2.ScheduleSubscription (clientApps2.Get (0), Seconds (3.5), star.GetSpokeIpv4Address (0));

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("NotificationCount.dat");
  clientApps.Get (0)->TraceConnectWithoutContext ("RxNotification", MakeBoundCallback (&LogRecieved, stream));
  clientApps2.Get (0)->TraceConnectWithoutContext ("RxNotification", MakeBoundCallback (&LogRecieved, stream));

  stream = asciiTraceHelper.CreateFileStream ("SentCount.dat");
  clientApps.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&LogSent, stream));
  clientApps2.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&LogSent, stream));
  serverApps.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&LogSent, stream));

  stream = asciiTraceHelper.CreateFileStream ("AckCount.dat");
  serverApps.Get (0)->TraceConnectWithoutContext ("Ack", MakeBoundCallback (&LogAck, stream));

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

  // Simulator must be stopped when using energy
  Simulator::Stop (Seconds (10.0));

  Simulator::Run ();

  for (DeviceEnergyModelContainer::Iterator iter = deviceModels.Begin (); iter != deviceModels.End (); iter++)
    {
      double energyConsumed = (*iter)->GetTotalEnergyConsumption ();
      NS_LOG_UNCOND ("End of simulation (" << Simulator::Now ().GetSeconds ()
                                           << "s) Total energy consumed by radio = " << energyConsumed << "J");
    }

  Simulator::Destroy ();
  return 0;
}

void disconnect (Ptr<NetDevice> router)
{
  auto csmaRouter = DynamicCast<CsmaNetDevice, NetDevice> (router);
  csmaRouter->SetSendEnable (false);
  NS_LOG_INFO ("Network Disconnected");
}

/// Trace function for remaining energy at node.
void
RemainingEnergy (Ptr<OutputStreamWrapper> stream, double oldValue, double remainingEnergy)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s Current remaining energy = " << remainingEnergy << "J");
  if (remainingEnergy <= 0)
    {
      total_dead_battery++;
    }

  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << total_dead_battery << std::endl;
}

/// Trace function for total energy consumption at node.
void
TotalEnergy (Ptr<OutputStreamWrapper> stream, double oldValue, double totalEnergy)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds ()
               << "s Total energy consumed by radio = " << totalEnergy << "J");

  total_power += totalEnergy - oldValue;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << total_power << std::endl;
}

static void
LogRecieved (Ptr<OutputStreamWrapper> stream)
{
  ++notifications_received;
  NS_LOG_INFO (Simulator::Now ().GetSeconds () << "\t" << notifications_received);
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << notifications_received << std::endl;
}

static void
LogSent (Ptr<OutputStreamWrapper> stream)
{
  ++total_sent_messages;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << total_sent_messages << std::endl;
}

static void
LogAck (Ptr<OutputStreamWrapper> stream)
{
  ++total_notification_acks;
  *stream->GetStream () << Simulator::Now ().GetSeconds () << "\t" << total_notification_acks << std::endl;
}
