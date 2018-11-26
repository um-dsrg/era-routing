/*
 * links.cc
 *
 *  Created on: Nov 26, 2018
 *      Author: noel
 */
#include <iostream>
#include <sstream>

#include "links.h"

Links::Links (tinyxml2::XMLNode* rootNode)
{
  PopulateLinksFromXml(rootNode);
}

/**
 * Retrieves the cost of the link
 *
 * An out of range exception is thrown if the link is not found.
 *
 * @param linkId The link Id
 * @return The link's cost
 */
double
Links::GetLinkCost (id_t linkId)
{
  try {
      return m_links.at(linkId).cost;
  } catch (std::out_of_range &e) {
      std::cerr << "Link with ID: " << linkId << " NOT found" << std::endl;
      std::cerr << "Error message: " << e.what() << std::endl;
      throw;
  }
}

/**
 * Retrieves the capacity of the link
 *
 * An out of range exception is thrown if the link is not found.
 *
 * @param linkId The link Id
 * @return The link's capacity
 */
double
Links::GetLinkCapacity (id_t linkId)
{
  try{
        return m_links.at(linkId).capacity;
    } catch (std::out_of_range &e) {
        std::cerr << "Link with ID: " << linkId << " NOT found" << std::endl;
        std::cerr << "Error message: " << e.what() << std::endl;
        throw;
    }
}

void
Links::PopulateLinksFromXml (tinyxml2::XMLNode* rootNode)
{
  using namespace tinyxml2;

  XMLElement* linkElement = rootNode->FirstChildElement("LinkDetails")->FirstChildElement("Link");

  while (linkElement != nullptr)
    {
      id_t linkId;
      linkElement->QueryAttribute("Id", &linkId);

      Link link;
      linkElement->QueryAttribute("Cost", &link.cost);
      linkElement->QueryAttribute("Capacity", &link.capacity);

      auto ret = m_links.insert(std::pair<id_t, Link>(linkId, link));
      if (ret.second == false)
	{
	  std::stringstream ss;
	  ss << "Duplicate link found. Id: " << linkId;
	  throw std::runtime_error(ss.str());
	}

      linkElement = linkElement->NextSiblingElement("Link");
    }
}
