/*
 * path.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */
#include "path.h"

Path::Path (tinyxml2::XMLElement* pathElement)
{
  using namespace tinyxml2;

  pathElement->QueryAttribute("Cost", &m_cost);
  pathElement->QueryAttribute("Id", &m_id);
}
