#include <iostream>
#include <tinyxml2.h>

#include "links.h"
#include "flows.h"

int main (int argc, const char *argv[])
{
  std::string kspXmlPath {"/home/noel/Documents/GDrive/Scratchpad/genetic_algorithm/butterfly_tcp/butterfly_ksp.xml"};
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

  // Build the links
  Links links{rootNode};
  std::map<id_t, Flow> flows {PopulateFlowsFromXml(rootNode)};

  return EXIT_SUCCESS;
}
