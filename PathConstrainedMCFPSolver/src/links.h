/*
 * links.h
 *
 *  Created on: Nov 26, 2018
 *      Author: noel
 */

#ifndef SRC_LINKS_H_
#define SRC_LINKS_H_

#include <map>
#include <tinyxml2.h>

#include "definitions.h"

class Links
{
public:
  Links(tinyxml2::XMLNode* rootNode);

  double GetLinkCost(id_t linkId);
  double GetLinkCapacity(id_t linkId);

private:
  void PopulateLinksFromXml(tinyxml2::XMLNode* rootNode);

  struct Link
  {
    double cost;
    double capacity;
  };
  std::map<id_t, Link> m_links;
};

#endif /* SRC_LINKS_H_ */
