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

Flow::Flow (const Flow& flow): m_id(flow.m_id), m_requestedDataRate(flow.m_requestedDataRate),
  m_allocatedDataRate(flow.m_allocatedDataRate), m_paths(flow.m_paths)
{
}

void
Flow::calculateAllocatedDataRate (const lemon::GlpkLp& lpSolver)
{
  for (Path* path: m_paths)
  {
    m_allocatedDataRate += lpSolver.primal(path->getDataRateLpVar());
  }
}
