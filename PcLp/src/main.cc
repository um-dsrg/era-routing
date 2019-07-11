#include <iostream>
#include <tinyxml2.h>
#include <lemon/lp.h>
#include <boost/program_options.hpp>

#include "lp_solver.h"
#include "definitions.h"
#include "xml_handler.h"
#include "path_file_parser.h"

namespace po = boost::program_options;

int
main (int argc, const char *argv[])
{
  std::string kspXmlPath {""};
  std::string resultXmlPath {""};
  std::string optimisationProblem{""};

  try
  {
    po::options_description cmdLineParams("Allowed Options");
    cmdLineParams.add_options()
                  ("help", "Help Message")
                  ("input", po::value<std::string>(&kspXmlPath)->required(),
                   "The path to XML result file generated by KSP")
                  ("output", po::value<std::string>(&resultXmlPath)->required(),
                   "The path where to store the result file")
                  ("optimisationProblem", po::value<std::string>(&optimisationProblem)->required(),
                   "The name of the optimisation problem to run. Available names are:"
                   "MaxFlow_MinCost | MaxFlow_FlowLimitedMinCost | MaxFlow_MaxDelay");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, cmdLineParams), vm);

    if (vm.count("help")) // Output help message
    {
      std::cout << cmdLineParams << "\n";
      return EXIT_SUCCESS;
    }
    po::notify(vm); // Check if all required parameters are passed

    XmlHandler xmlHandler (kspXmlPath);
    tinyxml2::XMLNode* rootNode = xmlHandler.getKspRootNode();

    linkContainer_t links;
    pathContainer_t paths;
    flowContainer_t flows;

    // Build the necessary data structures from the KSP result file
    parsePathFile(rootNode, links, paths, flows);

    LpSolver lpSolver (links, paths, flows);

    auto optimalSolutionFound = lpSolver.SolveProblem(optimisationProblem);

    if (!optimalSolutionFound)
    {
      std::cerr << "Optimal Solution NOT found" << std::endl;
      return EXIT_FAILURE;
    }

    // Save the results in the XML file!
    xmlHandler.saveResults(links, paths, flows, lpSolver, resultXmlPath);
  }
  catch (std::exception& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
    return EXIT_FAILURE;
  } catch (...)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
