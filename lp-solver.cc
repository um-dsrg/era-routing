#include <iostream>
#include <tinyxml2.h>

#include "flow-manager.h"
#include "graph-manager.h"

int main(int argc, char *argv[])
{
  // TODO: ADD Command line parameter parsing.
  const std::string lgfPath ("/home/noel/Development/source-code/lp-solver/sample.lgf");
  const std::string xmlLogPath ("/home/noel/Development/source-code/lp-solver/log.xml");
  try
    {
      tinyxml2::XMLDocument xmlLogFile;

      FlowManager flowManager;
      flowManager.LoadFlowsFromFile(lgfPath);
      GraphManager graphManager (flowManager.GetFlows(), &xmlLogFile);
      graphManager.ParseGraph(lgfPath);
      graphManager.FindOptimalSolution();
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
