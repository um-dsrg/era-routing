/*
 * link.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */
#include "link.h"

Link::Link (tinyxml2::XMLElement *linkElement)
{
  linkElement->QueryAttribute ("Id", &m_id);
  linkElement->QueryAttribute ("Cost", &m_cost);
  linkElement->QueryAttribute ("Capacity", &m_capacity);
}
