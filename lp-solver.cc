#include <iostream>
#include <tinyxml2.h>

#include "flow-manager.h"
#include "graph-manager.h"
#include "xml-utilities.h"

int main(int argc, char *argv[])
{
  // TODO: ADD Command line parameter parsing.
  const std::string lgfPath ("/home/noel/Development/source-code/lp-solver/sample.lgf");
  const std::string xmlLogPath ("/home/noel/Development/source-code/lp-solver/log.xml");
  try
    {
      FlowManager flowManager;
      flowManager.LoadFlowsFromFile(lgfPath);
      GraphManager graphManager (flowManager.GetFlows());
      graphManager.ParseGraph(lgfPath);
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
