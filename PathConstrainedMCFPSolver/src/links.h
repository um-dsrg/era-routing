/*
 * links.h
 *
 *  Created on: Nov 26, 2018
 *      Author: noel
 */

#ifndef SRC_LINKS_H_
#define SRC_LINKS_H_

#include <map>
#include <vector>
#include <lemon/lp.h>
#include <tinyxml2.h>

#include "definitions.h"

class Links
{
public:
  struct Link
  {
    double cost;
    double capacity;
    std::vector<lemon::Lp::Col> pathVariables;
  };
  std::map<id_t, Link> m_links;

  Links(tinyxml2::XMLNode* rootNode);

  double GetLinkCost(id_t linkId);
  double GetLinkCapacity(id_t linkId);
  std::vector<lemon::Lp::Col>& GetPaths (id_t linkId);

  void AddPathToLink(id_t linkId, lemon::Lp::Col pathVariable);

private:
  void PopulateLinksFromXml(tinyxml2::XMLNode* rootNode);
};

#endif /* SRC_LINKS_H_ */
