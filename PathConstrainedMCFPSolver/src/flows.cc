///*
// * flows.cc
// *
// *  Created on: Nov 26, 2018
// *      Author: noel
// */
//#include <string>
//#include <sstream>
//
//#include "flows.h"
//
//Path::Path (tinyxml2::XMLElement* pathElement)
//{
//  using namespace tinyxml2;
//
//  // Set the path cost
//  pathElement->QueryAttribute("Cost", &m_cost);
//
//  // Populate the links used by this path
//  XMLElement* linkElement = pathElement->FirstChildElement("Link");
//  while (linkElement != nullptr)
//    {
//      id_t linkId;
//      linkElement->QueryAttribute("Id", &linkId);
//      m_links.push_back(linkId);
//
//      linkElement = linkElement->NextSiblingElement("Link");
//    }
//}
//
//Flow::Flow (tinyxml2::XMLElement* flowElement)
//{
//  PopulateFlowFromXmlElement(flowElement);
//}
//
//void
//Flow::PopulateFlowFromXmlElement(tinyxml2::XMLElement* flowElement)
//{
//  using namespace tinyxml2;
//
//  flowElement->QueryAttribute("Id", &m_id);
//  flowElement->QueryAttribute("DataRate", &m_dataRate);
//
//  XMLElement* pathElement = flowElement->FirstChildElement("Paths")->FirstChildElement("Path");
//
//  while (pathElement != nullptr)
//    {
//      id_t pathId;
//      pathElement->QueryAttribute("Id", &pathId);
//      Path path(pathElement);
//
//      auto ret = m_paths.insert(std::pair<id_t, Path> (pathId, path));
//      if (ret.second == false)
//	{
//	  std::stringstream ss;
//	  ss << "Duplicate path found. Id: " << pathId;
//	  throw std::runtime_error(ss.str());
//	}
//
//      pathElement = pathElement->NextSiblingElement("Path");
//    }
//}
//
//std::map<id_t, Flow>
//PopulateFlowsFromXml (tinyxml2::XMLNode* rootNode)
//{
//  using namespace tinyxml2;
//  std::map<id_t, Flow> flows;
//  XMLElement* flowElement = rootNode->FirstChildElement("FlowDetails")->FirstChildElement("Flow");
//
//  while (flowElement != nullptr)
//    {
//      std::string flowProtocol {flowElement->Attribute("Protocol")};
//      if (flowProtocol == "T" || flowProtocol == "U") // Only parse TCP/UDP flows
//	{
//	  id_t flowId;
//	  flowElement->QueryAttribute("Id", &flowId);
//
//	  Flow flow(flowElement);
//
//	  auto ret = flows.insert(std::pair<id_t, Flow> (flowId, flow));
//	  if (ret.second == false)
//	    {
//	      std::stringstream ss;
//	      ss << "Duplicate flow found. Id: " << flowId;
//	      throw std::runtime_error(ss.str());
//	    }
//	}
//
//      flowElement = flowElement->NextSiblingElement("Flow");
//    }
//
//  return flows;
//}
//
//
