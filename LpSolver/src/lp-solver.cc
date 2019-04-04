#include <iostream>
#include <tinyxml2.h>
#include <boost/program_options.hpp>

#include "flow-manager.h"
#include "graph-manager.h"
#include "xml-utilities.h"

namespace po = boost::program_options;

int main (int argc, const char *argv[])
{
    std::string solverConfiguration {""};
    std::string lgfPath {""};
    std::string xmlLogPath {""};

  try
  {
    po::options_description cmdLineParams("Allowed Options");
    cmdLineParams.add_options()
      ("help", "Help Message")
      ("lgfPath", po::value<std::string>(&lgfPath)->required(),
       "The path to the LGF file")
      ("xmlLogPath", po::value<std::string>(&xmlLogPath)->required(),
       "The path where to store the XML log file with the optimal solution")
      ("solverConfig",
       po::value<std::string>(&solverConfiguration)->default_value("mfmc"),
       "The solver configuration\n"
       "mfmc: MaxFlowMinCost\nmf: Max Flow\nmc: Minimum Cost");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdLineParams), vm);

    if (vm.count("help")) // Output help message
    {
      std::cout << cmdLineParams << "\n";
      return EXIT_SUCCESS;
    }
    po::notify(vm); // Check if all required parameters are passed

    FlowManager flowManager;
    flowManager.LoadFlowsFromFile (lgfPath);

    GraphManager graphManager (flowManager.GetFlows());
    graphManager.ParseGraph (lgfPath);
    graphManager.VerifyFlows ();
    graphManager.FindOptimalSolution (solverConfiguration);

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
  catch (std::exception& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  catch (...)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
