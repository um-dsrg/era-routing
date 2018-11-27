#include <iostream>
#include <tinyxml2.h>
#include <lemon/lp.h>

#include "links.h"
#include "flows.h"

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

  // Build the links
  Links links{rootNode};
  std::map<id_t, Flow> flows {PopulateFlowsFromXml(rootNode)};

  lemon::GlpkLp lpSolver;

  // Assign an LP variable for each path
  for (auto& flowTuple : flows)
    {
      for (auto& pathTuple: flowTuple.second.GetPaths())
        {
          lemon::Lp::Col dataRateOnPath = lpSolver.addCol();
          pathTuple.second.SetAssignedDataRateVariable(dataRateOnPath);

          // Constraint that the data rate assigned on each path is positive
          lpSolver.addRow(dataRateOnPath >= 0);
        }
    }

  // Add the paths that each link is being used by
  for (auto& flowTuple : flows)
    {
      for (auto& pathTuple: flowTuple.second.GetPaths())
        {
          Path& path{pathTuple.second};

          for (auto& linkId : path.GetLinks()) // Loop through all the links
            {
              links.AddPathToLink(linkId, path.GetAssignedDataRateVariable());
            }
        }
    }

  // Set the flow data rate constraint such that no flow is over provisioned
  for (auto& flowTuple : flows)
    {
      Flow& flow {flowTuple.second};
      lemon::Lp::Expr flowDataRateExpression;

      for (auto& pathTuple: flow.GetPaths())
        flowDataRateExpression += pathTuple.second.GetAssignedDataRateVariable();

      lpSolver.addRow(flowDataRateExpression <= flow.GetDataRate());
    }

  // Link capacity constraint
  for (auto& linkTuple: links.m_links)
    {
      lemon::Lp::Expr linkCapacityExpression;
      for (auto& pathVariable: linkTuple.second.pathVariables)
        {
          linkCapacityExpression += pathVariable;
        }
      lpSolver.addRow(linkCapacityExpression <= linkTuple.second.capacity);
    }

  // Set the solver to find the solution with the maximum value
  lpSolver.max();

  // Set the objective
  lemon::Lp::Expr objective;

  // Loop through all the flows, all the paths and sum all the variables together.
  for (auto& flowTuple : flows) // Loop through all the flows
    {
      for (auto& pathTuple: flowTuple.second.GetPaths())
        {
          objective += pathTuple.second.GetAssignedDataRateVariable();
        }
    }

  lpSolver.obj(objective);

  // Solve the problem
  lpSolver.solve();

  // Output the solution to the problem
  if (lpSolver.primalType() != lemon::Lp::OPTIMAL)
    {
      std::cout << "Solution *NOT* found" << std::endl;
      return EXIT_FAILURE;
    }

  std::cout << "Solution results" << std::endl;
  std::cout << "----------------" << std::endl;

  for (auto& flowTuple : flows) // Loop through all the flows
    {
      std::cout << "Flow Id: " << flowTuple.first << std::endl;

      for (auto& pathTuple: flowTuple.second.GetPaths())
        {
          id_t pathId {pathTuple.first};
          double dataRate {lpSolver.primal(pathTuple.second.GetAssignedDataRateVariable())};
          std::cout << "  Path Id: " << pathId << " Data Rate: " << dataRate << std::endl;
        }
    }

  return EXIT_SUCCESS;
}
