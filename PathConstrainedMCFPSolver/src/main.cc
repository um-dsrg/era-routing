#include <iostream>
#include <tinyxml2.h>
#include <lemon/lp.h>

#include "definitions.h"
#include "ksp_parser.h"
#include "lp_solver.h"

int main (int argc, const char *argv[])
{
  std::string kspXmlPath {"/home/noel/Documents/GDrive/Scratchpad/pc_mcfp/butterfly_udp/butterfly_ksp.xml"};
  tinyxml2::XMLDocument kspXmlFile;
  tinyxml2::XMLError eResult = kspXmlFile.LoadFile(kspXmlPath.c_str());

  if (eResult != tinyxml2::XML_SUCCESS)
    {
      std::cerr << "File could not be parsed" << std::endl;
      return EXIT_FAILURE;
    }

  tinyxml2::XMLNode* rootNode = kspXmlFile.FirstChildElement("Log");
  if (rootNode == nullptr)
    {
      std::cerr << "Could not find the root <Log> element in the given XML file" << std::endl;
      return EXIT_FAILURE;
    }

  linkContainer_t links;
  pathContainer_t paths;
  flowContainer_t flows;

  // Build the necessary data structures from the KSP result file
  parseKspData(rootNode, links, paths, flows);

  LpSolver lpSolver (links, paths, flows);
  lpSolver.solve();

  std::cout << "Solution results" << std::endl;
  std::cout << "----------------" << std::endl;

  for (std::unique_ptr<Flow>& flow: flows)
    {
      std::cout << "Flow Id: " << flow->getId() << std::endl;

      for (Path* path : flow->getPaths())
        {
          std::cout << "  Path Id: " << path->getId()
                    << " Data Rate: " << lpSolver.getLpColValue(path->getDataRateLpVar())
                    << std::endl;
        }
    }

  return EXIT_SUCCESS;
}
