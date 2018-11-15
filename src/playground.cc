#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

inline void playground_enable_logging()
{
  LogComponentEnable("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable("UdpServer", LOG_LEVEL_INFO);
}

inline void playground_build_topology()
{
  NodeContainer nodes;
  nodes.Create(2);

  InternetStackHelper internetStackHelper;
  internetStackHelper.Install(nodes);

  PointToPointHelper pointToPointHelper;
  NetDeviceContainer netDevices = pointToPointHelper.Install(nodes);

  Ipv4AddressHelper addrHelper("10.10.0.0", "255.255.255.0");
  Ipv4InterfaceContainer intfContainer = addrHelper.Assign(netDevices);

  UdpServerHelper server(6565);
  ApplicationContainer serverAppContainer = server.Install(nodes.Get(0));
  serverAppContainer.Start(Seconds(0.0));
  serverAppContainer.Stop(Days(1.0));

  UdpClientHelper client(intfContainer.GetAddress(0), 6565);
  client.SetAttribute("MaxPackets", UintegerValue(999999));
  client.SetAttribute("Interval", TimeValue(Seconds(1.0)));
  client.SetAttribute("PacketSize", UintegerValue(1500U));
  ApplicationContainer clientAppContainer = client.Install(nodes.Get(1));
  clientAppContainer.Start(Seconds(1.0));
  clientAppContainer.Stop(Days(1.0));
}
