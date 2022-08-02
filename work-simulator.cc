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

// Default Network topology, 29 nodes in a star connected to a router,
// and the router connected to a local server
/*
          n0 n1 n2...
           \ | /
            \|/
       s0---ap0
            /| \
           / | \
          n26 n27 n28
*/
// - Primary traffic goes from the nodes to the local server through the AP
// - The local server responds to the node through the AP

#include "sys/stat.h"
#include "sys/types.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "ns3/applications-module.h"
#include "ns3/bridge-helper.h"
#include "ns3/core-module.h"
#include "ns3/csma-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/flow-monitor.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/work-device-enforcer.h"
#include "ns3/work-server.h"
#include "ns3/work-utils.h"

#define NUMBER_OF_DEVICES 29
#define ACTIVITY_COL 29
#define DATE_COL 30

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("EXAMPLE");

void appsConfiguration(Ipv4InterfaceContainer serverApInterface, double start,
                       double stop, NodeContainer serverNode,
                       NodeContainer staNodes,
                       Ipv4InterfaceContainer staInterface, string dataRate) {
  // Create a server to receive these packets
  // Start at 0s
  // Stop at final
  Address serverAddress(
      InetSocketAddress(serverApInterface.GetAddress(0, 0), 50000));
  Ptr<WorkServer> WorkServerApp = CreateObject<WorkServer>();
  WorkServerApp->SetAttribute("Protocol",
                              TypeIdValue(TcpSocketFactory::GetTypeId()));
  WorkServerApp->SetAttribute("Local", AddressValue(serverAddress));
  WorkServerApp->SetStartTime(Seconds(start));
  WorkServerApp->SetStopTime(Seconds(stop));
  serverNode.Get(0)->AddApplication(WorkServerApp);

  // Start at 1s
  // Each start 0.2s apart
  // Stop at final
  double startDevice = start + 1.0;
  for (uint32_t i = 0; i < staNodes.GetN(); ++i) {
    Address nodeAddress(
        InetSocketAddress(staInterface.GetAddress(i, 0), 50000));
    Ptr<DeviceEnforcer> DeviceEnforcerApp = CreateObject<DeviceEnforcer>();
    DeviceEnforcerApp->SetAttribute("Protocol",
                                    TypeIdValue(TcpSocketFactory::GetTypeId()));
    DeviceEnforcerApp->SetAttribute("Local", AddressValue(nodeAddress));
    DeviceEnforcerApp->SetAttribute("Remote", AddressValue(serverAddress));
    DeviceEnforcerApp->SetAttribute("DataRate",
                                    DataRateValue(DataRate(dataRate)));
    DeviceEnforcerApp->SetStartTime(Seconds(startDevice));
    DeviceEnforcerApp->SetStopTime(Seconds(stop));
    startDevice += 0.2;

    Ptr<Node> node = staNodes.Get(i);
    node->AddApplication(DeviceEnforcerApp);
    NS_LOG_INFO("Installed device " << i);
  }
}

int main(int argc, char *argv[]) {
  //----------------------------------------------------------------------------------
  // Simulation logs
  //----------------------------------------------------------------------------------
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  LogComponentEnable("EXAMPLE", LOG_LEVEL_ALL);
  LogComponentEnable("DeviceEnforcer", LOG_LEVEL_ALL);
  LogComponentEnable("WorkServer", LOG_LEVEL_ALL);
  // LogComponentEnable("ArpL3Protocol", LOG_LEVEL_INFO);

  //----------------------------------------------------------------------------------
  // Simulation variables
  //----------------------------------------------------------------------------------
  // Set up some default values for the simulation.
  // The below value configures the default behavior of global routing.
  // By default, it is disabled.  To respond to interface events, set to true
  Config::SetDefault("ns3::Ipv4GlobalRouting::RespondToInterfaceEvents",
                     BooleanValue(true));
  double start = 0.0;
  // double stop = 86400.0;
  double stop = 200.0;
  uint32_t N = NUMBER_OF_DEVICES; // number of nodes in the star
  uint32_t payloadSize = 1448;    /* Transport layer payload size in bytes. */
  string dataRate = "100Mbps";    /* Application layer datarate. */
  string phyRate = "HtMcs7";      /* Physical layer bitrate. */
  double simulationTime = stop;   /* Simulation time in seconds. */
  bool pcapTracing = false;       /* PCAP Tracing is enabled or not. */

  // Allow the user to override any of the defaults and the above
  // Config::SetDefault()s at run-time, via command-line arguments
  CommandLine cmd(__FILE__);
  cmd.AddValue("nNodes", "Number of nodes to place in the star", N);
  cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue("dataRate", "Application data ate", dataRate);
  cmd.AddValue("phyRate", "Physical layer bitrate", phyRate);
  cmd.AddValue("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue("pcap", "Enable/disable PCAP Tracing", pcapTracing);
  cmd.Parse(argc, argv);

  // Config::SetDefault("ns3::TcpL4Protocol::SocketType",
  //                    TypeIdValue(TypeId::LookupByName("ns3::TcpNewReno")));

  /* Configure TCP Options */
  Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));

  // Config::SetDefault("ns3::ArpCache::DeadTimeout",
  //                    TimeValue(MilliSeconds(500)));
  // Config::SetDefault("ns3::ArpCache::WaitReplyTimeout",
  //                    TimeValue(MilliSeconds(200)));
  // Config::SetDefault("ns3::ArpCache::MaxRetries", UintegerValue(10));
  // Config::SetDefault("ns3::ArpCache::PendingQueueSize",
  //                    UintegerValue(NUMBER_OF_DEVICES));

  //----------------------------------------------------------------------------------
  // Topology configuration
  //----------------------------------------------------------------------------------

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  WifiStandard wifiStandard = WIFI_STANDARD_80211n_2_4GHZ;
  wifiHelper.SetStandard(wifiStandard);
  Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                     DoubleValue(40.046));

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  // wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  // wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel",
  // "Frequency",
  //                                DoubleValue(5e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel(wifiChannel.Create());
  // Set MIMO capabilities
  // wifiPhy.Set("Antennas", UintegerValue(4));
  // wifiPhy.Set("MaxSupportedTxSpatialStreams", UintegerValue(4));
  // wifiPhy.Set("MaxSupportedRxSpatialStreams", UintegerValue(4));
  // wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
  // wifiHelper.SetRemoteStationManager("ns3::AarfWifiManager");
  // wifiHelper.SetRemoteStationManager("ns3::IdealWifiManager");
  wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
                                     StringValue(phyRate), "ControlMode",
                                     StringValue("ErpOfdmRate24Mbps"));

  // Here, we will create N nodes in a star.
  NS_LOG_INFO("Create nodes.");
  uint32_t nServers = 1;
  uint32_t nAps = 1;
  NodeContainer serverNode;
  serverNode.Create(nServers);
  NodeContainer apNode;
  apNode.Create(nAps);
  NodeContainer staNodes;
  staNodes.Create(N);
  NodeContainer allNodes = NodeContainer(serverNode, apNode, staNodes);

  NodeContainer serverAp = NodeContainer(serverNode.Get(0), apNode.Get(0));
  CsmaHelper csma;
  csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
  csma.SetChannelAttribute("Delay", StringValue("1ms"));
  NetDeviceContainer serverApDevice = csma.Install(serverAp);

  /* Configure AP */
  NS_LOG_INFO("Configure AP");
  Ssid ssid = Ssid("network");
  wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

  NetDeviceContainer apDevice;
  apDevice = wifiHelper.Install(wifiPhy, wifiMac, apNode);

  /* Configure STA */
  NS_LOG_INFO("Configure STA");
  wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install(wifiPhy, wifiMac, staNodes);

  Config::Set("/NodeList//DeviceList//$ns3::WifiNetDevice/HtConfiguration/"
              "ShortGuardIntervalSupported",
              BooleanValue(true));

  NS_LOG_INFO("Configure Bridge");
  BridgeHelper bridge;
  NetDeviceContainer bridgeDev;
  bridgeDev =
      bridge.Install(apNode.Get(0), NetDeviceContainer(apDevice.Get(0),
                                                       serverApDevice.Get(1)));

  /* Mobility model */
  NS_LOG_INFO("Configure mobility");
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc =
      CreateObject<ListPositionAllocator>();
  positionAlloc->Add("data/positions.csv");

  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(allNodes);

  // Install network stacks on the nodes
  InternetStackHelper internet;
  internet.Install(allNodes);

  // Later, we add IP addresses.
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4AddressHelper address;
  address.SetBase("192.168.0.0", "255.255.255.0");
  Ipv4InterfaceContainer serverApInterface =
      address.Assign(serverApDevice.Get(0));
  Ipv4InterfaceContainer apInterface = address.Assign(bridgeDev);
  Ipv4InterfaceContainer staInterface = address.Assign(staDevices);

  // Turn on global static routing
  // Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  //----------------------------------------------------------------------------------
  // Applications configuration
  //----------------------------------------------------------------------------------

  appsConfiguration(serverApInterface, start, stop, serverNode, staNodes,
                    staInterface, dataRate);

  //----------------------------------------------------------------------------------
  // Output configuration
  //----------------------------------------------------------------------------------

  /* Enable Traces */
  if (pcapTracing) {
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    wifiPhy.EnablePcap("AccessPoint", apDevice);
    wifiPhy.EnablePcap("Station", staDevices);
    csma.EnablePcap("Server", serverApDevice);
  }

  // configure tracing
  AsciiTraceHelper ascii;
  // mobility.EnableAsciiAll(ascii.CreateFileStream("trace.tr"));
  csma.EnableAsciiAll(ascii.CreateFileStream("trace.tr"));

  /* Stop Simulation */
  Simulator::Stop(Seconds(simulationTime + 1));

  // 40mx28m
  AnimationInterface anim("animation.xml");
  anim.SetBackgroundImage(
      "/home/grosa/Dev/ns-allinone-3.35/ns-3.35/data/home-design.png", 0, 0,
      0.07, 0.07, 1.0);
  for (uint32_t i = 0; i < staNodes.GetN(); ++i) {
    Ptr<Node> node = staNodes.Get(i);
    anim.UpdateNodeDescription(node, to_string(i)); // Optional
    anim.UpdateNodeColor(node, 255, 0, 0);          // Optional
    anim.UpdateNodeSize(node->GetId(), 0.8, 0.8);
  }
  for (uint32_t i = 0; i < apNode.GetN(); ++i) {
    Ptr<Node> node = apNode.Get(i);
    anim.UpdateNodeDescription(node, "AP"); // Optional
    anim.UpdateNodeColor(node, 0, 255, 0);  // Optional
    anim.UpdateNodeSize(node->GetId(), 0.8, 0.8);
  }
  for (uint32_t i = 0; i < serverNode.GetN(); ++i) {
    Ptr<Node> node = serverNode.Get(i);
    anim.UpdateNodeDescription(node, "Local Server"); // Optional
    anim.UpdateNodeColor(node, 0, 0, 255);            // Optional
    anim.UpdateNodeSize(node->GetId(), 1.2, 1.2);
  }
  anim.EnablePacketMetadata();
  anim.EnableIpv4RouteTracking("anim.txt", Seconds(0), Seconds(200),
                               Seconds(5));

  // Flow monitor
  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  // Trace routing tables
  // Ipv4GlobalRoutingHelper g;
  // Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(
  //     "dynamic-global-routing.routes", std::ios::out);
  // g.PrintRoutingTableAllAt(Seconds(12), routingStream);

  //----------------------------------------------------------------------------------
  // Call simulation
  //----------------------------------------------------------------------------------

  NS_LOG_INFO("Run Simulation.");
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Done.");

  flowMonitor->SerializeToXmlFile("flow.xml", true, true);

  return 0;
}
