#include <thread>
#include <chrono>
#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartGridSim");

void controlSimulationTime(const int maxSimulationTime, const int maxWallClockTimeInMinutes);

int main(int argc, char *argv[])
{
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
  lteHelper->SetEpcHelper(epcHelper);

  Ptr<Node> pgw = epcHelper->GetPgwNode();

  NodeContainer routerNodeContainer;
  routerNodeContainer.Create(1);
  Ptr<Node> routerHost = routerNodeContainer.Get(0);
  InternetStackHelper internetHelper;
  internetHelper.Install(routerNodeContainer);

  // Create the Internet
  PointToPointHelper pgwRouterPointToPointHelper;
  NetDeviceContainer internetDevices = pgwRouterPointToPointHelper.Install(pgw, routerHost);

  Ipv4AddressHelper coreNetworkAddressHelper;
  coreNetworkAddressHelper.SetBase("10.101.0.0", "255.255.0.0");
  Ipv4InterfaceContainer internetIpIfaces = coreNetworkAddressHelper.Assign(internetDevices);
  Ipv4Address routerHostAddress = internetIpIfaces.GetAddress(1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> routerStaticRouting = ipv4RoutingHelper.GetStaticRouting(routerHost->GetObject<Ipv4>());
  routerStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(1);
  ueNodes.Create(2);

  // Install Mobility Model
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.InstallAll();

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice(enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice(ueNodes);

  // Install the IP stack on the UEs
  internetHelper.Install(ueNodes);
  Ipv4InterfaceContainer ueIpIface = epcHelper->AssignUeIpv4Address(ueLteDevs);
  for (uint32_t u = 0; u < ueNodes.GetN(); u++)
  {
    Ptr<Node> ueNode = ueNodes.Get(u);
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
  }

  //Attach all UEs to the eNB
  for (uint16_t i = 0; i < ueLteDevs.GetN(); i++)
    lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(0));

  //Configure the applications
  UdpServerHelper serverApp(6565);
  serverApp.Install(routerHost);

  UdpClientHelper client(routerHostAddress, 6565);
  client.Install(ueNodes);

  //Configure simulation output
  pgwRouterPointToPointHelper.EnablePcapAll("PGW-ROUTER");
  AnimationInterface anim("anim.xml");

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  std::thread simulationTimeController(controlSimulationTime, 30, 5);
  Simulator::Run();
  simulationTimeController.join();

  flowMonitor->SerializeToXmlFile("FlowMon.xml", true, true);
  Simulator::Destroy();

  return 0;
}

void controlSimulationTime(const int maxSimulationTime, const int maxWallClockTimeInMinutes)
{
  time_t startTime;

  Time simulationNow;
  Time maxSimulationNow;
  time_t wallNow;
  time_t maxWallNow;

  maxSimulationNow = Time::FromInteger(maxSimulationTime, Time::Unit::MIN);

  time(&maxWallNow);
  maxWallNow += maxWallClockTimeInMinutes * 60;

  time(&startTime);

  while ((simulationNow = Simulator::Now()) < maxSimulationNow && (wallNow = time(NULL)) < maxWallNow)
  {
    std::cout << "Simulation time of [" << simulationNow.ToInteger(Time::Unit::MIN)
              << "/"
              << maxSimulationNow.ToInteger(Time::Unit::MIN)
              << "] minutes; Elapsed wall clock time of ["
              << (wallNow - startTime) / 60
              << "/"
              << maxWallClockTimeInMinutes
              << "] minutes."
              << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  Simulator::Stop();
}
