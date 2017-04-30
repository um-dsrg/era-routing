#include <iostream>

#include "flow-manager.h"
#include "graph-manager.h"

int main()
{
  const std::string lgfPath ("/home/noel/Development/source-code/lp-solver/sample.lgf");
  try
    {
      FlowManager flowManager;
      flowManager.LoadFlowsFromFile(lgfPath);
      GraphManager graphManager (flowManager.GetFlows());
      graphManager.ParseGraph(lgfPath);
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
