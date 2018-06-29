#include <iostream>
#include <tinyxml2.h>

#include "flow-manager.h"
#include "graph-manager.h"
#include "xml-utilities.h"

int main (int argc, char *argv[])
{
  if (argc < 3 || argc > 4)
    {
      std::cerr << "Incorrect number of command-line arguments were passed." << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      /**
        * mfmc = Max Flow Minimum Cost
        * mf = Max Flow
        * mc = Minimum Cost
        */
      std::string solverConfiguration ("mfmc");
      const std::string lgfPath (argv[1]);
      const std::string xmlLogPath (argv[2]);
      if (argc == 4) solverConfiguration = argv[3];

      FlowManager flowManager;
      flowManager.LoadFlowsFromFile (lgfPath);

      GraphManager graphManager (flowManager.GetFlows());
      graphManager.ParseGraph (lgfPath);
      graphManager.VerifyFlows ();
      graphManager.FindOptimalSolution (solverConfiguration);
      graphManager.GenerateAckRoutes ();

      // XML Logging
      tinyxml2::XMLDocument xmlLogFile;
      XmlUtilities::InsertRootNode (xmlLogFile, "Log");
      XmlUtilities::InsertTimeStampInRootElement (xmlLogFile);
      // Generating the Optimal Solution + Network Topology elements in the XML
      // log file
      graphManager.AddLogsInXmlFile (xmlLogFile);

      // Save the XML file.
      XmlUtilities::SaveXmlFile (xmlLogPath, xmlLogFile);

      // Throw an error if the optimal solution is not found.
      if (!graphManager.OptimalSolutionFound()) return EXIT_FAILURE;
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
