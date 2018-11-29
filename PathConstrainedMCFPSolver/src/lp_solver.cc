/*
 * lp_solver.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#include "lp_solver.h"

LpSolver::LpSolver (linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows) : m_links (links), m_paths(paths), m_flows(flows)
{}

bool
LpSolver::solve ()
{
  bool maxFlow = solveMaxFlowProblem(); // Solve MaxFlow Problem
  if (maxFlow == false) return false;

  // Update the flow data rates
  for (std::unique_ptr<Flow>& flow: m_flows)
    flow->calculateAllocatedDataRate(m_lpSolver);

  m_lpSolver.clear(); // Reset the LP solver

  return solveMinCostProblem(); // Solve MinCost Problem
}

void
LpSolver::assignLpVariablePerPath ()
{
  // TODO Move in documentation stubble:
  for (std::unique_ptr<Path>& path: m_paths)
    {
      lemon::Lp::Col dataRateOnPath = m_lpSolver.addCol();
      path->setDataRateLpVar(dataRateOnPath);
      m_lpSolver.addRow(dataRateOnPath >= 0); // Ensure non-negative data rate assignment
    }
}

void
LpSolver::setFlowDataRateConstraint (bool allowReducedFlowRate)
{
  // TODO Move in documentation stubble: Set the flow data rate constraint such that no flow is over provisioned
  for (std::unique_ptr<Flow>& flow: m_flows)
    {
      lemon::Lp::Expr flowDataRateExpression;

      for (Path* path: flow->getPaths())
        flowDataRateExpression += path->getDataRateLpVar();

      if (allowReducedFlowRate)
        m_lpSolver.addRow(flowDataRateExpression <= flow->getRequestedDataRate());
      else
        m_lpSolver.addRow(flowDataRateExpression == flow->getAllocatedDataRate());
    }
}

void
LpSolver::setLinkCapacityConstraint ()
{
  // TODO Move in documentation stubble: Link capacity constraint
  for (auto& linkTuple : m_links)
    {
      std::unique_ptr<Link>& link {linkTuple.second};

      lemon::Lp::Expr linkCapacityExpression;
      for (Path* path: link->getPaths())
        {
          linkCapacityExpression += path->getDataRateLpVar();
        }

      m_lpSolver.addRow(linkCapacityExpression <= link->getCapacity());
    }
}

void
LpSolver::setMaxFlowObjective ()
{
  // TODO Move in documentation stubble:
  // Set the solver to find the solution with the maximum value
  lemon::Lp::Expr maxFlowObjective;

  for (std::unique_ptr<Path>& path: m_paths)
    {
      maxFlowObjective += path->getDataRateLpVar();
    }

  m_lpSolver.max();
  m_lpSolver.obj(maxFlowObjective);
}

void
LpSolver::setMinCostObjective ()
{
  lemon::Lp::Expr minCostObjective;

  for (std::unique_ptr<Path>& path: m_paths)
    minCostObjective += (path->getDataRateLpVar() * path->getCost());

  m_lpSolver.min();
  m_lpSolver.obj(minCostObjective);
}

bool
LpSolver::solveMaxFlowProblem ()
{
  assignLpVariablePerPath();
  setFlowDataRateConstraint(/*allowReducedFlowRate*/ true);
  setLinkCapacityConstraint();
  setMaxFlowObjective();

  // TODO Create function to call the solver and store the timing.
  m_lpSolver.solve();

  if (m_lpSolver.primalType() == lemon::Lp::OPTIMAL)
    return true;
  else
    return false;
}

bool
LpSolver::solveMinCostProblem ()
{
  assignLpVariablePerPath();
  setFlowDataRateConstraint(/*allowReducedFlowRate*/ false);
  setLinkCapacityConstraint();
  setMinCostObjective();

  // TODO Create function to call the solver and store the timing.
  m_lpSolver.solve();

  if (m_lpSolver.primalType() == lemon::Lp::OPTIMAL)
    return true;
  else
    return false;
}
