/*
 * flow.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#include "flow_details.h"

Path::Path (tinyxml2::XMLElement* pathElement)
{
  using namespace tinyxml2;

  pathElement->QueryAttribute("Cost", &m_cost);
  pathElement->QueryAttribute("Id", &m_id);
}

Flow::Flow (tinyxml2::XMLElement* flowElement)
{
  using namespace tinyxml2;

  flowElement->QueryAttribute("Id", &m_id);
  flowElement->QueryAttribute("DataRate", &m_dataRate);
}

void
populateFlowAndPathDetails (tinyxml2::XMLNode* rootNode, flowContainer_t& flows, pathContainer_t& paths)
{
  using namespace tinyxml2;
  XMLElement* flowElement = rootNode->FirstChildElement("FlowDetails")->FirstChildElement("Flow");

  while (flowElement != nullptr)
    {
      std::string flowProtocol {flowElement->Attribute("Protocol")};
      if (flowProtocol == "T" || flowProtocol == "U") // Only parse TCP/UDP flows
        {
          Flow flow(flowElement);

          XMLElement* pathElement = flowElement->FirstChildElement("Paths")->FirstChildElement("Path");

          while (pathElement != nullptr)
            {
              Path path(pathElement);
              flow.addPath(&path);
              paths.push_back(path);

              pathElement = pathElement->NextSiblingElement("Path");
            }
          flows.push_back(flow);
        }

      flowElement = flowElement->NextSiblingElement("Flow");
    }
}
