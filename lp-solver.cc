#include <iostream>
#include <tinyxml2.h>

#include "flow-manager.h"
#include "graph-manager.h"
#include "xml-utilities.h"

int main(int argc, char *argv[])
{
  if (argc != 3)
    {
      std::cerr << "Incorrect number of command-line arguments were passed." << std::endl;
      return EXIT_FAILURE;
    }

  try
    {
      const std::string lgfPath (argv[1]);
      const std::string xmlLogPath (argv[2]);

      FlowManager flowManager;
      flowManager.LoadFlowsFromFile(lgfPath);
      GraphManager graphManager (flowManager.GetFlows());
      graphManager.ParseGraph(lgfPath);
      graphManager.VerifyFlows();
      graphManager.FindOptimalSolution();

      // XML Logging
      tinyxml2::XMLDocument xmlLogFile;
      XmlUtilities::InsertRootNode(xmlLogFile, "Log");
      XmlUtilities::InsertTimeStampInRootElement(xmlLogFile);
      // Generating the Optimal Solution + Network Topology elements in the XML log file
      graphManager.AddLogsInXmlFile(xmlLogFile);

      // Save the XML file.
      XmlUtilities::SaveXmlFile(xmlLogPath, xmlLogFile);
    }
  catch (...)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}
