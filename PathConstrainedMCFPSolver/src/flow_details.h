/*
 * flow.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_FLOW_DETAILS_H_
#define SRC_FLOW_DETAILS_H_

#include <tinyxml2.h>
#include <lemon/lp.h>
#include "definitions.h"

class Path
{
public:
  /**
   * Constructs a Path object from a Path XML element
   * @param pathElement The XML Path element
   */
  Path (tinyxml2::XMLElement* pathElement);

  /* Setters */
  void setDataRateLpVar (lemon::Lp::Col lpVar) { m_assignedDataRate = lpVar; }

  /* Getters */
  id_t getId () { return m_id; }
  double getCost () { return m_cost; }
  lemon::Lp::Col getDataRateLpVar () { return m_assignedDataRate; }

private:
  id_t m_id;
  double m_cost;
  lemon::Lp::Col m_assignedDataRate;
};

class Flow
{
public:
  /**
   * Constructs a Flow object from a Flow XML element
   * @param pathElement The XML Flow element
   */
  Flow (tinyxml2::XMLElement* flowElement);

  /* Getters */
  double getDataRate () { return m_dataRate; }

  void addPath (Path* path) { m_paths.push_back(path); }

private:
  id_t m_id;
  double m_dataRate;
  std::vector<Path*> m_paths;
};

void populateFlowAndPathDetails (tinyxml2::XMLNode* rootNode, flowContainer_t& flows, pathContainer_t& paths);

#endif /* SRC_FLOW_DETAILS_H_ */
