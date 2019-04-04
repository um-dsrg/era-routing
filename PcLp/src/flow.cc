/*
 * flow.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#include "flow.h"

Flow::Flow (tinyxml2::XMLElement* flowElement) : m_allocatedDataRate {0.0}
{
  using namespace tinyxml2;

  flowElement->QueryAttribute("Id", &m_id);
  flowElement->QueryAttribute("RequestedDataRate", &m_requestedDataRate);
}

void
Flow::calculateAllocatedDataRate (const lemon::GlpkLp& lpSolver)
{
  for (Path* path: m_paths)
    {
      m_allocatedDataRate += lpSolver.primal(path->getDataRateLpVar());
    }
}
