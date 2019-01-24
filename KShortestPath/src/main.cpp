#include <iostream>
#include <boost/program_options.hpp>

#include "flow.hpp"
#include "boost-graph.hpp"
#include "xml-handler.hpp"

namespace po = boost::program_options;

int main(int argc, const char * argv[])
{
  uint32_t k {0};
  std::string lgfPath;
  std::string kspXmlPath;

  try
    {
      po::options_description cmdLineParams("Allowed options");
      cmdLineParams.add_options()
      ("help", "produce help message")
      ("lgfPath", po::value<std::string>(&lgfPath)->required(),
       "The path to the LGF graph file")
      ("k", po::value<uint32_t>(&k)->required(),
       "Number of shortest paths to calculate")
      ("kspXmlPath", po::value<std::string>(&kspXmlPath)->required(),
       "The path where to store the output of the KSP algorithm in XML format."
       "This file is used by the Genetic Algorithm to build the network and "
       "paths");

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, cmdLineParams), vm);

      if (vm.count("help")) // Output help message
        {
          std::cout << cmdLineParams << "\n";
          return 0;
        }

      po::notify(vm); // Check if all parameters required were passed

      LemonGraph lemonGraph;
      lemonGraph.LoadGraphFromFile (lgfPath);

      BoostGraph boostGraph (lemonGraph);
      boostGraph.GenerateBoostGraphFromLemonGraph (lemonGraph);

      Flow::flowContainer_t flows = ParseFlows (lgfPath);

      boostGraph.AddKShortestPaths (flows, k);

      XmlHandler kspFile;
      kspFile.AddParameterList (lgfPath, k);
      kspFile.AddLinkDetails (lemonGraph);
      kspFile.AddFlows (flows, lemonGraph);
      kspFile.SaveXmlFile (kspXmlPath);
    }
  catch (std::exception& e)
    {
      std::cerr << "Error: " << e.what() << "\n";
      return -1;
    }
  catch (...)
    {
      std::cerr << "Error occurred" << "\n";
      return -1;
    }

  return 0;
}
