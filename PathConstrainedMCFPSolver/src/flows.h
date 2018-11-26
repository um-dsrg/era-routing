/*
 * flows.h
 *
 *  Created on: Nov 26, 2018
 *      Author: noel
 */

#ifndef SRC_FLOWS_H_
#define SRC_FLOWS_H_

#include <vector>
#include <map>
#include <tinyxml2.h>
#include <lemon/lp.h>

#include "definitions.h"

class Path {
public:
  Path(tinyxml2::XMLElement* pathElement);

  lemon::Lp::Col GetAssignedDataRateVariable() { return m_assignedDataRate; }
  void SetAssignedDataRateVariable(lemon::Lp::Col dataRateVariable) { m_assignedDataRate = dataRateVariable; }

private:
  double m_cost;
  std::vector<id_t> m_links;
  lemon::Lp::Col m_assignedDataRate;
};

class Flow {
public:
  Flow(tinyxml2::XMLElement* flowElement);

  double GetDataRate() { return m_dataRate; }
  std::map<id_t, Path>& GetPaths() { return m_paths; }

private:
  void PopulateFlowFromXmlElement(tinyxml2::XMLElement* flowElement);

  id_t m_id;
  double m_dataRate;
  std::map<id_t, Path> m_paths;
};

std::map<id_t, Flow> PopulateFlowsFromXml (tinyxml2::XMLNode* rootNode);

#endif /* SRC_FLOWS_H_ */
