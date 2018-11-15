#include "ns3/core-module.h"
#include "src/playground.cc"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SmartGridSimulation");

int main(int argc, char *argv[])
{
  playground_enable_logging();
  playground_build_topology();

  NS_LOG_INFO("Run Simulation.");
  Simulator::Run();
  Simulator::Destroy();
  NS_LOG_INFO("Done.");

  return 0;
}
