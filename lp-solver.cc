#include <iostream>

#include "flow-manager.h"

int main()
{
  try
    {
      FlowManager flowManager;
      flowManager.LoadFlowsFromFile("/home/noel/Development/source-code/lp-solver/sample.lgf");
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
