/*
 * link.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_LINK_H_
#define SRC_LINK_H_

#include <map>
#include <vector>
#include <tinyxml2.h>

#include "definitions.h"
#include "flow_details.h"

class Link
{
public:
  /**
   * Constructs a Link object from a Link XML element
   * @param linkElement The XML Link element
   */
  Link (tinyxml2::XMLElement* linkElement);

  /* Getters */
  double getCost () const { return m_cost; }
  double getCapacity () const { return m_capacity; }
  const std::vector<Path*>& getPaths () const { return m_paths; }

  void addPath (Path* path) { m_paths.push_back(path); }

private:
  id_t m_id;
  double m_cost;
  double m_capacity;
  std::vector<Path*> m_paths;
};

void populateLinks (tinyxml2::XMLNode* rootNode, linkContainer_t& links);

#endif /* SRC_LINK_H_ */
