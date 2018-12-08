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
#include "ns3/flow-monitor.h"
#include "ns3/geographic-positions.h"

#define NUMBER_OF_DEVICES 100
#define NUMBER_OF_ENBS 27

#define CENTER_LAT -3.749886
#define CENTER_LNG -38.528574

#define SIMULATION_TIME 5
#define SIMULATION_CLOCK_TIME 300

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartGridSim");

Ptr<ListPositionAllocator> generateRandomPositionAllocatorAroundCenter(int n, int z, int radius);
void controlSimulationTime(const int maxSimulationTime, const int maxWallClockTimeInMinutes);

int main(int argc, char *argv[])
{
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
  Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(5)); //Transmission Mode 5: MIMO Multi-User.

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
  lteHelper->SetSchedulerType("ns3::FdMtFfMacScheduler");

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
  pgwRouterPointToPointHelper.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gb/s"))); //gigabit
  pgwRouterPointToPointHelper.SetDeviceAttribute("Mtu", UintegerValue(9000));                   //jumbo frames
  pgwRouterPointToPointHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(3.33)));      //considering a cable of 1km length
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
  enbNodes.Create(NUMBER_OF_ENBS);
  ueNodes.Create(NUMBER_OF_DEVICES);

  // Setup the mobility model for the smart meters
  MobilityHelper smartMetersMobilityHelper;
  smartMetersMobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  smartMetersMobilityHelper.SetPositionAllocator(generateRandomPositionAllocatorAroundCenter(NUMBER_OF_DEVICES, 95, 1000));
  smartMetersMobilityHelper.Install(ueNodes);

  // Setup the mobility model for the enbs
  // TODO setup according to the actual positions
  MobilityHelper enbsMobilityHelper;
  enbsMobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  enbsMobilityHelper.SetPositionAllocator(generateRandomPositionAllocatorAroundCenter(NUMBER_OF_ENBS, 100, 1200));
  enbsMobilityHelper.Install(enbNodes);

  // Setup the mobility model to remaining nodes
  NodeContainer remainingNodes;
  remainingNodes.Add(pgw);
  remainingNodes.Add(routerHost);

  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityHelper.SetPositionAllocator(generateRandomPositionAllocatorAroundCenter(remainingNodes.GetN(), 100, 10));
  mobilityHelper.Install(remainingNodes);

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
  // lteHelper->Attach(ueLteDevs); // TODO make sure we are attaching according to some LTE algorithm
  for (uint16_t i = 0; i < ueLteDevs.GetN(); i++)
    lteHelper->Attach(ueLteDevs.Get(i), enbLteDevs.Get(i % enbLteDevs.GetN()));

  //Configure the applications
  UdpServerHelper serverApp(6565);
  serverApp.Install(routerHost);

  for (uint32_t i = 0; i < ueNodes.GetN(); i++)
  {
    UdpClientHelper client(routerHostAddress, 6565);
    client.SetAttribute("Interval", TimeValue(Seconds(rand() % 3 + 1)));
    client.SetAttribute("PacketSize", UintegerValue(100));
    ApplicationContainer appContainer = client.Install(ueNodes.Get(i));
    appContainer.Start(Seconds(rand() % 3 + 1));
  }

  //Configure simulation output
  pgwRouterPointToPointHelper.EnablePcapAll("PGW-ROUTER");

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  AnimationInterface anim("anim.xml");
  int pointsBetweenUeNodes = 10;
  for (uint32_t i = 0; i < ueNodes.GetN(); i++)
  {
    anim.SetConstantPosition(ueNodes.Get(i), pointsBetweenUeNodes * i + pointsBetweenUeNodes, 10);
    anim.UpdateNodeColor(ueNodes.Get(i), 0, 255, 0);
  }
  int pointsBetweenEnbNodes = pointsBetweenUeNodes * ueNodes.GetN() / enbNodes.GetN();
  for (uint32_t i = 0; i < enbNodes.GetN(); i++)
  {
    anim.SetConstantPosition(enbNodes.Get(i), pointsBetweenEnbNodes * i + pointsBetweenEnbNodes, 30);
    anim.UpdateNodeColor(enbNodes.Get(i), 0, 255, 255);
  }

  for (uint32_t i = 0; i < remainingNodes.GetN(); i++)
  {
    anim.SetConstantPosition(remainingNodes.Get(i), 10 * i + 10, 50);
  }

  //Run the simulation
  std::thread simulationTimeController(controlSimulationTime, SIMULATION_TIME, SIMULATION_CLOCK_TIME);
  Simulator::Run();
  simulationTimeController.join();

  flowMonitor->SerializeToXmlFile("FlowMon.xml", true, true);
  Simulator::Destroy();

  return 0;
}

void controlSimulationTime(const int maxSimulationTimeInSeconds, const int maxWallClockTimeInSeconds)
{
  time_t startTime;

  Time simulationNow;
  Time maxSimulationNow;
  time_t wallNow;
  time_t maxWallNow;

  maxSimulationNow = Time::FromInteger(maxSimulationTimeInSeconds, Time::Unit::S);

  time(&maxWallNow);
  maxWallNow += maxWallClockTimeInSeconds;

  time(&startTime);

  while ((simulationNow = Simulator::Now()) < maxSimulationNow && (wallNow = time(NULL)) < maxWallNow)
  {
    std::cout << "Simulation time of [" << simulationNow.ToInteger(Time::Unit::S)
              << "/"
              << maxSimulationNow.ToInteger(Time::Unit::S)
              << "] seconds; Elapsed wall clock time of ["
              << wallNow - startTime
              << "/"
              << maxWallClockTimeInSeconds
              << "] seconds."
              << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  Simulator::Stop();
  std::cout << "Stopped with simulation time of " << simulationNow.ToInteger(Time::Unit::S) << " seconds..." << std::endl;
}

Ptr<ListPositionAllocator> generateRandomPositionAllocatorAroundCenter(int n, int z, int radius)
{
  Ptr<UniformRandomVariable> randomVariable = CreateObject<UniformRandomVariable>();
  std::list<Vector> positions = GeographicPositions::RandCartesianPointsAroundGeographicPoint(
      CENTER_LAT,
      CENTER_LNG,
      z,
      n,
      radius,
      randomVariable);

  Ptr<ListPositionAllocator> positionAllocator = CreateObject<ListPositionAllocator>();

  for (std::list<Vector>::iterator it = positions.begin(); it != positions.end(); it++)
  {
    positionAllocator->Add(*it);
  }

  return positionAllocator;
}