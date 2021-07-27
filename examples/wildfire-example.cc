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
#include "ns3/lte-module.h"

#include "ns3/basic-energy-source.h"
#include "ns3/wifi-radio-energy-model.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/wifi-radio-energy-model-helper.h"
#include "ns3/energy-source-container.h"
#include "ns3/device-energy-model-container.h"

#include "ns3/netanim-module.h"

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
  uint32_t nNodes = 2;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("nNodes", "Number of nodes to create", nNodes);
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  LogComponentEnable ("WildfireClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("WildfireServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Creating Topology");

  /** LTE Model **/
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (10)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // enbNodes are LTE towers, ueNodes are mobile devices
  NodeContainer enbNodes;
  NodeContainer ueNodes;
  enbNodes.Create (2);
  ueNodes.Create (nNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < enbNodes.GetN (); i++)
    {
      positionAlloc->Add (Vector (0, 0, 0));
      i += 1;
      positionAlloc->Add (Vector (35000, 35000, 0));
    }

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (positionAlloc);
  mobility.Install (enbNodes);

  // Disconection from enb at 0,0 begins at 20000,20000
  mobility.SetPositionAllocator ("ns3::RandomDiscPositionAllocator",
                                 "X", StringValue ("20000.0"),
                                 "Y", StringValue ("20000.0"),
                                 "Rho", StringValue ("ns3::UniformRandomVariable[Min=-20000|Max=20000]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  mobility.Install (ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  // X2 Interface should enable handoff between enbs
  lteHelper->AddX2Interface (enbNodes);
  internet.Install (ueNodes);

  // Install the IP stack on the UEs
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (ueLteDevs);

  // Install the IP stack on the UEs
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach UEs to eNB
  lteHelper->AttachToClosestEnb (ueLteDevs, enbLteDevs);
  // side effect: the default EPS bearer will be activated


  /** Wifi Model **/
  NodeContainer wifiNodes;
  for (int i = 0; i < ueNodes.GetN (); ++i)
    {
      wifiNodes.Add (ueNodes.Get (i));
    }

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

  //End wifi related

  /** Animation **/
  AnimationInterface anim ("wildfire-animation.xml"); // Mandatory

  for (uint32_t i = 0; i < wifiNodes.GetN (); ++i)
    {
      anim.UpdateNodeDescription (wifiNodes.Get (i), "MOB"); // Optional
      anim.UpdateNodeColor (wifiNodes.Get (i), 255, 0, 0); // Optional
    }

  anim.UpdateNodeDescription (enbNodes.Get (0), "eNB"); // Optional
  anim.UpdateNodeColor (enbNodes.Get (0), 0, 255, 0); // Optional

  anim.UpdateNodeDescription (epcHelper->GetPgwNode (), "PGW"); // Optional
  anim.UpdateNodeColor (epcHelper->GetPgwNode (), 0, 0, 255); // Optional

  anim.UpdateNodeDescription (epcHelper->GetSgwNode (), "SGW"); // Optional
  anim.UpdateNodeColor (epcHelper->GetSgwNode (), 0, 0, 255); // Optional

  // Note: Node 2 is the MME node used for the LTE simulation. I am unable to
  // get a reference to that node at this time in order to rename it

  anim.UpdateNodeDescription (remoteHostContainer.Get (0), "Server"); // Optional
  anim.UpdateNodeColor (remoteHostContainer.Get (0), 0, 0, 255); // Optional
  AnimationInterface::SetConstantPosition (remoteHostContainer.Get (0), 10, 30);


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

  ApplicationContainer serverApps = echoServer.Install (remoteHostContainer.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));
  echoServer.ScheduleNotification (serverApps.Get (0), Seconds (5.0));

  WildfireClientHelper echoClient (remoteHostAddr, 202, 202);
  echoClient.SetAttribute ("BroadcastInterval", TimeValue (Seconds (2.0)));

  ApplicationContainer clientApps = echoClient.Install (wifiNodes);
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  for ( int i = 0; i < clientApps.GetN (); ++i)
    {
      echoClient.ScheduleSubscription (clientApps.Get (i), Seconds (2.5), remoteHostAddr );
    }

  AsciiTraceHelper asciiTraceHelper;
  Ptr<OutputStreamWrapper> stream = asciiTraceHelper.CreateFileStream ("NotificationCount.dat");
  for ( int i = 0; i < clientApps.GetN (); ++i)
    {
      clientApps.Get (i)->TraceConnectWithoutContext ("RxNotification", MakeBoundCallback (&LogRecieved, stream));
    }

  stream = asciiTraceHelper.CreateFileStream ("SentCount.dat");
  for ( int i = 0; i < clientApps.GetN (); ++i)
    {
      clientApps.Get (i)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&LogSent, stream));
    }

  serverApps.Get (0)->TraceConnectWithoutContext ("Tx", MakeBoundCallback (&LogSent, stream));

  stream = asciiTraceHelper.CreateFileStream ("AckCount.dat");
  serverApps.Get (0)->TraceConnectWithoutContext ("Ack", MakeBoundCallback (&LogAck, stream));

  // This allows for global routing across connection types
  // Currently causing a crash
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  AsciiTraceHelper ascii;
  //pointToPoint.EnableAsciiAll (ascii.CreateFileStream ("myfirst.tr"));

  // Schedule Network Disruption
  // TODO: Update this to disable LTE eNBs
  // Server (Internet)
  //Simulator::Schedule (Seconds (4.0), disconnect, star.GetHub ()->GetDevice (0));
  // Wireless Device 1
  //Simulator::Schedule (Seconds (4.0), disconnect, star.GetHub ()->GetDevice (1));
  // Wireless Device 2
  //Simulator::Schedule (Seconds (4.0), disconnect, star.GetHub ()->GetDevice (2));

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
