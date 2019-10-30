#include <iostream>
#include <boost/program_options.hpp>

#include "flow.hpp"
#include "boost-graph.hpp"
#include "xml-handler.hpp"

namespace po = boost::program_options;

int
main (int argc, const char *argv[])
{
  try
    {
      std::string inputFile{""};
      std::string outputFile{""};
      std::string pathSelectionAlgorithm{""};
      uint32_t globalK{0};
      bool perFlowK{false};
      bool verbose{false};

      po::options_description cmdLineParams ("Allowed options");
      // clang-format off
      cmdLineParams.add_options ()
        ("help,h", "Produce the help message.")
        ("input,i", po::value<std::string> (&inputFile)->required (),
         "The path to the LGF graph file.")
        ("output,o", po::value<std::string> (&outputFile)->required (),
         "The path where to store the output of the KSP algorithm in XML format.")
        ("pathSelectionAlgorithm", po::value<std::string> (&pathSelectionAlgorithm)->required(),
         "The path selection method to use. Available options are: KSP (K Shortest Path), "
         "RED (Releaxed Edge Disjoint), ED (Edge Disjoint)")
        ("globalK", po::value<uint32_t> (&globalK),
         "Number of shortest paths to calculate for every flow.")
        ("perFlowK", po::bool_switch (&perFlowK),
         "When set, the number of paths per flow will be determined based on the "
        "per flow k value.")
        ("verbose,v", po::bool_switch (&verbose), "Enable verbose output.");
      // clang-format on

      po::variables_map vm;
      po::store (po::parse_command_line (argc, argv, cmdLineParams), vm);

      if (vm.count ("help")) // Output help message
        {
          std::cout << cmdLineParams << "\n";
          return 0;
        }

      po::notify (vm); // Check if all parameters required were passed

      if (!perFlowK && globalK <= 0)
        throw std::runtime_error ("The global K value needs to be set if per flow K is disabled");

      LemonGraph lemonGraph (inputFile);
      BoostGraph boostGraph (lemonGraph);

      Flow::flowContainer_t flows{ParseFlows (inputFile, perFlowK, globalK)};

      boostGraph.AssignPathsToFlows (flows, pathSelectionAlgorithm);
      boostGraph.AddAckPaths (flows);
      boostGraph.AddShortestPathAck (flows);

      if (verbose)
        PrintFlows (flows);

      XmlHandler kspResFile;
      kspResFile.AddParameterList (inputFile, outputFile, globalK, perFlowK);
      kspResFile.AddLinkDetails (boostGraph);
      kspResFile.AddFlows (flows);
      kspResFile.AddNetworkTopology (boostGraph);
      kspResFile.SaveFile (outputFile);

  } catch (const std::exception &e)
    {
      std::cerr << "Error: " << e.what () << "\n";
      return -1;
  } catch (...)
    {
      std::cerr << "Error occurred"
                << "\n";
      return -1;
  }

  return 0;
}
