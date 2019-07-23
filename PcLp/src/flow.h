/*
 * flow.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_FLOW_H_
#define SRC_FLOW_H_

#include <vector>
#include <tinyxml2.h>
#include <lemon/lp.h>

#include "definitions.h"
#include "path.h"

class Flow
{
public:
  /**
   * Constructs a Flow object from a Flow XML element
   * @param pathElement The XML Flow element
   */
  Flow (tinyxml2::XMLElement* flowElement);
  Flow (const Flow& flow);

  void addPath (Path* path) { m_paths.push_back(path); }
  id_t getId () { return m_id; }
  double getRequestedDataRate () { return m_requestedDataRate; }
  double getAllocatedDataRate () { return m_allocatedDataRate; }
  std::vector<Path*>& getPaths () { return m_paths; }

  void calculateAllocatedDataRate (const lemon::GlpkLp& lpSolver);

private:
  id_t m_id;
  double m_requestedDataRate;
  double m_allocatedDataRate;
  std::vector<Path*> m_paths;
};

#endif /* SRC_FLOW_H_ */
