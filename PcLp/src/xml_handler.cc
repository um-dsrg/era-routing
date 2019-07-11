/*
 * xml_handler.cc
 *
 *  Created on: Nov 29, 2018
 *      Author: noel
 */

#include <sstream>
#include <iomanip>
#include <chrono>
#include "xml_handler.h"

using namespace tinyxml2;

void InsertTimeStampInRootElement (XMLElement* rootElement);
void SaveObjectives (XMLDocument& xmlDoc, XMLElement* rootElement, LpSolver& lpSolver);
void SaveDuration (XMLDocument& xmlDoc, XMLElement* rootElement, LpSolver& lpSolver);
void SaveLinkDetails (XMLDocument& xmlDoc, XMLElement* rootElement, linkContainer_t& links);
void SaveOptimalSolution (XMLDocument& xmlDoc, XMLElement* rootElement, flowContainer_t& flows,
                          LpSolver& lpSolver);

XmlHandler::XmlHandler (std::string kspXmlPath)
{
  XMLError eResult = m_kspXmlDoc.LoadFile(kspXmlPath.c_str());

  if (eResult != XML_SUCCESS)
    {
      std::stringstream ss;
      ss << "The file at: " << kspXmlPath << " could not be parsed";
      throw std::runtime_error(ss.str());
    }
}

tinyxml2::XMLNode*
XmlHandler::getKspRootNode ()
{
  tinyxml2::XMLNode* rootNode = m_kspXmlDoc.FirstChildElement("Log");

  if (rootNode == nullptr)
    throw std::runtime_error("Could not find the root <Log> element in the given XML file");

  return rootNode;
}

void
XmlHandler::saveResults (linkContainer_t& links, pathContainer_t& paths,
                         flowContainer_t& flows, LpSolver& lpSolver,
                         std::string resultXmlPath)
{
  XMLDocument xmlResFile;

  // Inserting declaration
  xmlResFile.InsertFirstChild(xmlResFile.NewDeclaration());

  // Inserting root element
  XMLElement* rootElement = xmlResFile.NewElement("Log");

  // Insert time stamp
  InsertTimeStampInRootElement(rootElement);

  // Save the objective values
  SaveObjectives(xmlResFile, rootElement, lpSolver);

  // Save the solver duration
  SaveDuration(xmlResFile, rootElement, lpSolver);

  // Save the link details element
  SaveLinkDetails(xmlResFile, rootElement, links);

  // Save the optimal solution element
  SaveOptimalSolution(xmlResFile, rootElement, flows, lpSolver);

  xmlResFile.InsertEndChild(rootElement);

  // Save the XML result file
  if (xmlResFile.SaveFile(resultXmlPath.c_str()) != XML_SUCCESS)
    throw std::ios_base::failure("Could not save the XML Log File");
}

void
InsertTimeStampInRootElement (XMLElement* rootElement)
{
  std::chrono::time_point<std::chrono::system_clock> currentTime (std::chrono::system_clock::now());
  std::time_t currentTimeFormatted = std::chrono::system_clock::to_time_t(currentTime);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&currentTimeFormatted), "%a %d-%m-%Y %T");

  rootElement->SetAttribute("Generated", ss.str().c_str());
}

void
SaveObjectives (XMLDocument& xmlDoc, XMLElement* rootElement, LpSolver& lpSolver)
{
  XMLElement* objectivesElement = xmlDoc.NewElement("Objectives");

  for (auto& [objectiveName, objectiveValue]: lpSolver.GetObjectiveValues())
  {
    XMLElement* objectiveElement = xmlDoc.NewElement("Objective");
    objectiveElement->SetAttribute("Name", objectiveName.c_str());
    objectiveElement->SetAttribute("Value", objectiveValue);
    objectivesElement->InsertEndChild(objectiveElement);
  }

  rootElement->InsertEndChild(objectivesElement);
}

void
SaveDuration (XMLDocument& xmlDoc, XMLElement* rootElement, LpSolver& lpSolver)
{
  auto totalDuration {0.0};
  XMLElement* durationElement = xmlDoc.NewElement("Duration");

  for (const auto& [optimisationProblemName, durationInMs]: lpSolver.GetTimings())
  {
    XMLElement* problemElement = xmlDoc.NewElement("OptimisationProblem");
    problemElement->SetAttribute("Name", optimisationProblemName.c_str());
    problemElement->SetAttribute("DurationMs", durationInMs);
    durationElement->InsertEndChild(problemElement);

    totalDuration += durationInMs;
  }

  durationElement->SetAttribute("TotalDurationMs", totalDuration);
  rootElement->InsertEndChild(durationElement);
}

void
SaveLinkDetails (XMLDocument& xmlDoc, XMLElement* rootElement, linkContainer_t& links)
{
  XMLElement* linkDetailsElement = xmlDoc.NewElement("LinkDetails");

  for (auto& linkTuple: links)
    {
      XMLElement* linkElement = xmlDoc.NewElement("Link");
      linkElement->SetAttribute("Id", linkTuple.first);
      linkElement->SetAttribute("Cost", linkTuple.second->getCost());
      linkElement->SetAttribute("Capacity", linkTuple.second->getCapacity());
      linkDetailsElement->InsertEndChild(linkElement);
    }

  rootElement->InsertEndChild(linkDetailsElement);
}

void
SaveOptimalSolution (XMLDocument& xmlDoc, XMLElement* rootElement, flowContainer_t& flows,
                     LpSolver& lpSolver)
{
  XMLElement* optSolnElement = xmlDoc.NewElement("OptimalSolution");

  for (std::unique_ptr<Flow>& flow: flows)
    {
      XMLElement* flowElement = xmlDoc.NewElement("Flow");
      flowElement->SetAttribute("Id", flow->getId());
      flowElement->SetAttribute("RequestedDataRate", flow->getRequestedDataRate());
      flowElement->SetAttribute("AllocatedDataRate", flow->getAllocatedDataRate());

      for (Path* path: flow->getPaths())
        {
          XMLElement* pathElement = xmlDoc.NewElement("Path");
          pathElement->SetAttribute("Id", path->getId());
          pathElement->SetAttribute("DataRate", lpSolver.GetLpColValue(path->getDataRateLpVar()));

          for (Link* link: path->getLinks())
            {
              XMLElement* linkElement = xmlDoc.NewElement("Link");
              linkElement->SetAttribute("Id", link->getId());
              pathElement->InsertEndChild(linkElement);
            }
          flowElement->InsertEndChild(pathElement);
        }

      optSolnElement->InsertEndChild(flowElement);
    }
  rootElement->InsertEndChild(optSolnElement);
}
