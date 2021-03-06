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
#include "ns3/config-store.h"
#include "ns3/buildings-helper.h"
#include "ns3/cost231-propagation-loss-model.h"

#define NUMBER_OF_ENBS 12

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartGridSim");

Ptr<ListPositionAllocator> getEnbsPositionAllocator();
Ptr<ListPositionAllocator> generateRandomPositionAllocatorAroundCenter(int n, int z, int radius);
void controlSimulationTime(const int maxSimulationTimeInSeconds, const int maxElapsedClockTimeInSeconds);

int main(int argc, char *argv[])
{
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);

  int numberOfSmartMeters = 10;
  int aggregationPercentage = 0;
  int maxSimulationTimeInSeconds = 10;
  int maxElapsedClockTimeInSeconds = 10;

  CommandLine cmd;
  cmd.AddValue("numberOfSmartMeters", "Number of Smart Meters", numberOfSmartMeters);
  cmd.AddValue("aggregationPercentage", "The aggregation percentage to be used in simulation", aggregationPercentage);
  cmd.AddValue("maxSimulationTimeInSeconds", "Max simulation time in seconds", maxSimulationTimeInSeconds);
  cmd.AddValue("maxElapsedClockTimeInSeconds", "Max elapsed clock time in seconds", maxElapsedClockTimeInSeconds);
  cmd.Parse(argc, argv);

  // if any of the params is set to 0, then nothing runs
  if (numberOfSmartMeters < 1 || maxSimulationTimeInSeconds < 1 || maxElapsedClockTimeInSeconds < 1)
    return 0;

  std::cout << "Number of Smart Meters: " << numberOfSmartMeters << std::endl;
  std::cout << "Aggregation percentage: " << aggregationPercentage << std::endl;
  std::cout << "Max simulation time in seconds: " << maxSimulationTimeInSeconds << std::endl;
  std::cout << "Max elapsed clock time in seconds: " << maxElapsedClockTimeInSeconds << std::endl
            << std::endl;

  numberOfSmartMeters = ((100 - aggregationPercentage) / 100.0) * numberOfSmartMeters / 60;
  if (numberOfSmartMeters < 1)
  {
    std::cout << "There's no smart meters allocated for this simulation, please provide feasible parameters" << std::endl;
    return 1;
  }

  std::cout << "Number of simultaneous transmissors: " << numberOfSmartMeters << std::endl;

  // Configure the LTE parameters
  Config::SetDefault("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue(320));
  Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(5)); //Transmission Mode 5: MIMO Multi-User.

  // UlBandwidth/DlBandwidth = Network bandwith (MHz)
  // UlBandwidth/DlBandwidth (25,50,75,100) = Network bandwith 5,10,15,20(MHz)
  Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(100));
  Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(100));

  // Power settings
  Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(49));
  Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(24));

  // Noise settings
  Config::SetDefault("ns3::LteEnbPhy::NoiseFigure", DoubleValue(5));
  Config::SetDefault("ns3::LteUePhy::NoiseFigure", DoubleValue(7));

  // DlEarfcn 100 = Downlink 2120(MHz)
  // UlEarfcn 18100 = Uplink 1930(MHz)
  Config::SetDefault("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue(100));
  Config::SetDefault("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue(18000));

  // Proportional fair scheduling
  Config::SetDefault("ns3::LteHelper::Scheduler", StringValue("ns3::PfFfMacScheduler"));

  Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
  Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.00005));

  Config::SetDefault("ns3::LteEnbRrc::EpsBearerToRlcMapping", EnumValue(ns3::LteEnbRrc::RLC_AM_ALWAYS));

  Config::SetDefault("ns3::LteHelper::UseCa", BooleanValue(true));
  Config::SetDefault("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue(2));
  Config::SetDefault("ns3::LteHelper::EnbComponentCarrierManager", StringValue("ns3::RrComponentCarrierManager"));

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
  lteHelper->SetHandoverAlgorithmType("ns3::NoOpHandoverAlgorithm");
  lteHelper->SetFfrAlgorithmType("ns3::LteFfrSoftAlgorithm");
  lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(1));

  // Propagation loss model
  lteHelper->SetAttribute("PathlossModel", StringValue("ns3::Cost231PropagationLossModel"));

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

  NodeContainer smartMeterNodes;
  NodeContainer enbNodes;
  enbNodes.Create(3 * NUMBER_OF_ENBS); //each base station has 3 antennas
  smartMeterNodes.Create(numberOfSmartMeters);

  // Setup the mobility model for the smart meters
  MobilityHelper smartMetersMobilityHelper;
  smartMetersMobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  smartMetersMobilityHelper.SetPositionAllocator(generateRandomPositionAllocatorAroundCenter(numberOfSmartMeters, 95, 1000));
  smartMetersMobilityHelper.Install(smartMeterNodes);
  BuildingsHelper::Install(smartMeterNodes);

  // Setup the mobility model for the enbs
  MobilityHelper enbsMobilityHelper;
  enbsMobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  enbsMobilityHelper.SetPositionAllocator(getEnbsPositionAllocator());
  enbsMobilityHelper.Install(enbNodes);
  BuildingsHelper::Install(enbNodes);

  // Setup the mobility model for remaining nodes
  NodeContainer remainingNodes;
  remainingNodes.Add(pgw);
  remainingNodes.Add(routerHost);

  MobilityHelper mobilityHelper;
  mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobilityHelper.SetPositionAllocator(generateRandomPositionAllocatorAroundCenter(remainingNodes.GetN(), 100, 10));
  mobilityHelper.Install(remainingNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs;
  for (uint32_t i = 0; i < enbNodes.GetN(); i += 3)
  {
    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(0));
    lteHelper->SetEnbAntennaModelAttribute("Beamwidth", DoubleValue(120));
    lteHelper->SetEnbAntennaModelAttribute("MaxGain", DoubleValue(17.0));
    lteHelper->SetFfrAlgorithmType("ns3::LteFfrSoftAlgorithm");
    lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(1));
    enbLteDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(i)));

    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(360 / 3));
    lteHelper->SetEnbAntennaModelAttribute("Beamwidth", DoubleValue(120));
    lteHelper->SetEnbAntennaModelAttribute("MaxGain", DoubleValue(17.0));
    lteHelper->SetFfrAlgorithmType("ns3::LteFfrSoftAlgorithm");
    lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(2));
    enbLteDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(i + 1)));

    lteHelper->SetEnbAntennaModelType("ns3::CosineAntennaModel");
    lteHelper->SetEnbAntennaModelAttribute("Orientation", DoubleValue(2 * 360 / 3));
    lteHelper->SetEnbAntennaModelAttribute("Beamwidth", DoubleValue(120));
    lteHelper->SetEnbAntennaModelAttribute("MaxGain", DoubleValue(17.0));
    lteHelper->SetFfrAlgorithmType("ns3::LteFfrSoftAlgorithm");
    lteHelper->SetFfrAlgorithmAttribute("FrCellTypeId", UintegerValue(3));
    enbLteDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(i + 2)));
  }

  lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
  NetDeviceContainer smartMeterLteDevs = lteHelper->InstallUeDevice(smartMeterNodes);

  // Install the IP stack on the UEs
  internetHelper.Install(smartMeterNodes);
  Ipv4InterfaceContainer smartMetersIpIface = epcHelper->AssignUeIpv4Address(smartMeterLteDevs);
  for (uint32_t u = 0; u < smartMeterNodes.GetN(); u++)
  {
    Ptr<Node> ueNode = smartMeterNodes.Get(u);
    Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
  }

  //Attach all UEs to the eNB
  lteHelper->Attach(smartMeterLteDevs);

  //Configure the applications
  UdpServerHelper serverApp(6565);
  serverApp.Install(routerHost);

  Ptr<UniformRandomVariable> randomVariable = CreateObject<UniformRandomVariable>();
  for (uint32_t i = 0; i < smartMeterNodes.GetN(); i++)
  {
    uint pktSize = randomVariable->GetInteger(100, 100 * 10 * (1 - aggregationPercentage / 100.0));
    uint interval = randomVariable->GetInteger(10, 100);

    UdpClientHelper client(routerHostAddress, 6565);
    client.SetAttribute("MaxPackets", UintegerValue(maxSimulationTimeInSeconds));
    client.SetAttribute("Interval", TimeValue(MilliSeconds(interval)));
    client.SetAttribute("PacketSize", UintegerValue(pktSize));
    ApplicationContainer appContainer = client.Install(smartMeterNodes.Get(i));
    appContainer.Start(MilliSeconds(500));
  }

  //Configure simulation output
  // pgwRouterPointToPointHelper.EnablePcapAll("PGW-ROUTER");

  Ptr<FlowMonitor> flowMonitor;
  FlowMonitorHelper flowHelper;
  flowMonitor = flowHelper.InstallAll();

  // AnimationInterface anim("anim.xml");
  // for (uint32_t i = 0; i < smartMeterNodes.GetN(); i++)
  // {
  //   anim.UpdateNodeColor(smartMeterNodes.Get(i), 0, 255, 0);
  // }
  // for (uint32_t i = 0; i < enbNodes.GetN(); i++)
  // {
  //   anim.UpdateNodeColor(enbNodes.Get(i), 0, 255, 255);
  // }

  //Run the simulation
  std::thread simulationTimeController(controlSimulationTime, maxSimulationTimeInSeconds, maxElapsedClockTimeInSeconds);
  Simulator::Run();
  simulationTimeController.join();

  //End of simulation
  flowMonitor->CheckForLostPackets();
  flowMonitor->SerializeToXmlFile("FlowMon.xml", true, true);

  Simulator::Destroy();

  return 0;
}

void controlSimulationTime(const int maxSimulationTimeInSeconds, const int maxElapsedClockTimeInSeconds)
{
  time_t startTime;

  Time simulationNow;
  Time maxSimulationNow;
  time_t wallNow;
  time_t maxWallNow;

  maxSimulationNow = Time::FromInteger(maxSimulationTimeInSeconds, Time::Unit::S);

  time(&maxWallNow);
  maxWallNow += maxElapsedClockTimeInSeconds;

  time(&startTime);

  while ((simulationNow = Simulator::Now()) < maxSimulationNow && (wallNow = time(NULL)) < maxWallNow)
  {
    std::cout << "Simulation time of [" << simulationNow.ToInteger(Time::Unit::S)
              << "/"
              << maxSimulationNow.ToInteger(Time::Unit::S)
              << "] seconds; Elapsed wall clock time of ["
              << wallNow - startTime
              << "/"
              << maxElapsedClockTimeInSeconds
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
      -3.749886,
      -38.528574,
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

Ptr<ListPositionAllocator> getEnbsPositionAllocator()
{
  double coordinates[NUMBER_OF_ENBS][2] = {
      {-3.755000, -38.523889},
      {-3.753333, -38.518056},
      {-3.746389, -38.522778},
      {-3.745833, -38.526944},
      {-3.743333, -38.519722},
      {-3.740278, -38.526111},
      {-3.738056, -38.534722},
      {-3.740833, -38.539722},
      {-3.747778, -38.538611},
      {-3.744722, -38.534722},
      {-3.750000, -38.530833},
      {-3.757778, -38.533333}};

  Ptr<ListPositionAllocator> positionAllocator = CreateObject<ListPositionAllocator>();
  for (int i = 0; i < NUMBER_OF_ENBS; i++)
  {
    for (int j = 0; j < 3; j++)
    {
      positionAllocator->Add(GeographicPositions::GeographicToCartesianCoordinates(
          coordinates[i][0],
          coordinates[i][1],
          100,
          GeographicPositions::EarthSpheroidType::SPHERE));
    }
  }

  return positionAllocator;
}
