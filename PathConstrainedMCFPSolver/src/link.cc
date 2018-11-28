/*
 * link.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#include <sstream>
#include "link.h"

Link::Link (tinyxml2::XMLElement* linkElement)
{
  linkElement->QueryAttribute("Id", &m_id);
  linkElement->QueryAttribute("Cost", &m_cost);
  linkElement->QueryAttribute("Capacity", &m_capacity);
}

void
populateLinks (tinyxml2::XMLNode* rootNode, linkContainer_t& links)
{
  using namespace tinyxml2;

  XMLElement* linkElement = rootNode->FirstChildElement("LinkDetails")->FirstChildElement("Link");

  while (linkElement != nullptr)
    {
      id_t linkId;
      linkElement->QueryAttribute("Id", &linkId);
      Link link (linkElement);

      auto ret = links.insert(std::pair<id_t, Link>(linkId, link));
      if (ret.second == false)
        {
          std::stringstream ss;
          ss << "Duplicate link found. Id: " << linkId;
          throw std::runtime_error(ss.str());
        }

      linkElement = linkElement->NextSiblingElement("Link");
    }
}
