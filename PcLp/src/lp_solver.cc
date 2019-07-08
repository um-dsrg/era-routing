/*
 * lp_solver.cc
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#include <chrono>
#include <algorithm>

#include "lp_solver.h"

LpSolver::LpSolver (linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows) :
  m_links (links), m_paths(paths), m_flows(flows), m_maxFlowDurationMs(0), m_minCostDurationMs (0)
{}

bool
LpSolver::solve ()
{
  bool maxFlow = solveMaxFlowProblem(); // Solve MaxFlow Problem
  if (maxFlow == false) return false;

  for (std::unique_ptr<Flow>& flow: m_flows) // Update the flow data rates
    flow->calculateAllocatedDataRate(m_lpSolver);

  m_lpSolver.clear(); // Reset the LP solver

  return solveMaxPathDelayProblem(); // Solve the MaxPathDelay problem
  // return solveMinCostProblem(); // Solve MinCost Problem
}

void
LpSolver::assignLpVariablePerPath ()
{
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
  lemon::Lp::Expr maxFlowObjective;

  for (std::unique_ptr<Path>& path: m_paths)
    {
      maxFlowObjective += path->getDataRateLpVar();
    }

  m_lpSolver.max(); // Set solver the find the solution with the largest value
  m_lpSolver.obj(maxFlowObjective);
}

void
LpSolver::setMinCostObjective ()
{
  lemon::Lp::Expr minCostObjective;

  for (std::unique_ptr<Path>& path: m_paths)
    minCostObjective += (path->getDataRateLpVar() * path->getCost());

  m_lpSolver.min(); // Set solver the find the solution with the smallest value
  m_lpSolver.obj(minCostObjective);
}

/**
  * Retrieves the lowest cost of the path set for a given flow
  */
double
getLowestPathCost(const std::vector<Path*>& flowPaths)
{
  std::vector<double> pathCost;

  for (const auto& path: flowPaths)
  {
    pathCost.push_back(path->getCost());
  }

  return *std::min_element(std::begin(pathCost), std::end(pathCost));
}

void
LpSolver::setMaxPathDelayMetricObjective()
{
  lemon::Lp::Expr maxPathDelayObjective;

  for (const auto& flow: m_flows)
  {
    lemon::Lp::Expr flowMetricValue;

    auto flowPaths = flow->getPaths();

    auto lowestPathCost = getLowestPathCost(flowPaths);

    for (const auto& path: flowPaths)
    {
      auto pathCost = path->getCost();
      auto pathMultiplier = double{1 / ((pathCost - lowestPathCost) + 1)};

      flowMetricValue += (pathMultiplier * path->getDataRateLpVar());
    }

    // Normalise the path objective such that each flow can have a value between
    // 0 and 1
    maxPathDelayObjective += (flowMetricValue / flow->getAllocatedDataRate());
  }

  m_lpSolver.max(); // Maximise the objective value
  m_lpSolver.obj(maxPathDelayObjective);
}

bool
LpSolver::solveLpProblem (Problem problem)
{
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  m_lpSolver.solvePrimalExact();
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

  if (problem == Problem::MaxFlow)
    m_maxFlowDurationMs = std::chrono::duration_cast<std::chrono::milliseconds> (end - start).count();
  else if (problem == Problem::MinCost)
    m_minCostDurationMs = std::chrono::duration_cast<std::chrono::milliseconds> (end - start).count();

  if (m_lpSolver.primalType() == lemon::Lp::OPTIMAL)
    return true;
  else
    return false;
}

bool
LpSolver::solveMaxFlowProblem ()
{
  assignLpVariablePerPath();
  setFlowDataRateConstraint(/*allowReducedFlowRate*/ true);
  setLinkCapacityConstraint();
  setMaxFlowObjective();

  return solveLpProblem(Problem::MaxFlow);
}

bool
LpSolver::solveMinCostProblem ()
{
  assignLpVariablePerPath();
  setFlowDataRateConstraint(/*allowReducedFlowRate*/ false);
  setLinkCapacityConstraint();
  setMinCostObjective();

  return solveLpProblem(Problem::MinCost);
}

bool
LpSolver::solveMaxPathDelayProblem ()
{
  assignLpVariablePerPath();
  setFlowDataRateConstraint(/*allowReducedFlowRate*/ false);
  setLinkCapacityConstraint();
  setMaxPathDelayMetricObjective();

  // TODO Update the below
  return solveLpProblem(Problem::MinCost);
}
