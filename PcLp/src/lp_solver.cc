#include <chrono>
#include <math.h>
#include <algorithm>
#include <lemon/lp_base.h>

#include "lp_solver.h"

/**
 * @brief Retrieve the lowest path cost from a given set of paths
 *
 * @param flowPaths The set of paths related to a flow
 */
double
getLowestPathCost (const std::vector<Path *> &flowPaths)
{
  std::vector<double> pathCost;

  for (const auto &path : flowPaths)
    {
      pathCost.push_back (path->getCost ());
    }

  return *std::min_element (std::begin (pathCost), std::end (pathCost));
}

LpSolver::LpSolver (linkContainer_t &links, pathContainer_t &paths)
    : m_links (links), m_paths (paths)
{
  m_lpSolver.messageLevel (lemon::LpBase::MessageLevel::MESSAGE_VERBOSE);
}

void
LpSolver::SetFlows (flowContainer_t *flowContainer)
{
  m_flows = flowContainer;
}

void
LpSolver::ClearTimings ()
{
  auto maxFlowDuration = m_timings["Maximum Flow"];
  m_timings.clear ();

  m_timings.emplace ("Maximum Flow", maxFlowDuration);
}

void
LpSolver::UpdateFlowAllocatedDataRate ()
{
  for (auto &flow : *m_flows)
    {
      flow->calculateAllocatedDataRate (m_lpSolver);
    }
}

bool
LpSolver::MinCost (double maxNetworkFlow)
{
  m_lpSolver.clear (); // Reset the LP solver

  auto [minCostSolnFound, minNetworkCost] =
      solveMinCostProblem (false /* Flow Limited Minimum Cost */, maxNetworkFlow);

  UpdateFlowAllocatedDataRate ();

  m_objectiveValues.emplace ("Maximum Flow", maxNetworkFlow);
  m_objectiveValues.emplace ("Minimum Cost", minNetworkCost);

  return minCostSolnFound;
}

bool
LpSolver::FlowLimitedMinCost (double maxNetworkFlow)
{
  // Update the Flow Allocated Data Rate
  for (std::unique_ptr<Flow> &flow : *m_flows)
    flow->calculateAllocatedDataRate (m_lpSolver);

  // Reset the LP solver
  m_lpSolver.clear ();

  auto [minCostSolnFound, minNetworkCost] =
      solveMinCostProblem (true /*Flow limited Minimum Cost*/);

  m_objectiveValues.emplace ("Maximum Flow", maxNetworkFlow);
  m_objectiveValues.emplace ("Minimum Cost", minNetworkCost);

  return minCostSolnFound;
}

bool
LpSolver::MaxDelayDistributionMetric (double maxNetworkFlow)
{
  UpdateFlowAllocatedDataRate ();

  // Reset the LP solver
  m_lpSolver.clear ();

  auto [maxDelaySolnFound, maxDelayDistributionMetric] = solveMaxDelayDistributionProblem ();

  m_objectiveValues.emplace ("Maximum Flow", maxNetworkFlow);
  m_objectiveValues.emplace ("Maximum Delay Distribution Metric", maxDelayDistributionMetric);

  return maxDelaySolnFound;
}

std::pair<bool, double>
LpSolver::solveMaxFlowProblem ()
{
  /* Variable assignments + Constraints */
  assignLpVariablePerPath ();
  setFlowDataRateConstraint (true /* Allow reduced Flow Rate */);
  setLinkCapacityConstraint ();

  /* Objective */
  setMaxFlowObjective ();

  auto [optimalSolutionFound, maxNetworkFlow] = solveLpProblem ("Maximum Flow");
  maxNetworkFlow = (floor (maxNetworkFlow * 10) / 10);

  m_objectiveValues.emplace ("Maximum Flow", maxNetworkFlow);

  return std::make_pair (optimalSolutionFound, maxNetworkFlow);
}

std::pair<bool, double>
LpSolver::solveMinCostProblem (bool flowLimitedMinCost, double totalNetworkFlow)
{
  /* Variable assignments + Constraints */
  assignLpVariablePerPath ();
  setFlowDataRateConstraint (!flowLimitedMinCost /* Allow reduced flow rate */);
  setLinkCapacityConstraint ();

  if (flowLimitedMinCost == false)
    setTotalNetworkFlowConstraint (totalNetworkFlow);

  /* Objective */
  setMinCostObjective ();

  return solveLpProblem ("Minimum Cost");
}

std::pair<bool, double>
LpSolver::solveMaxDelayDistributionProblem ()
{
  /* Variable assignments + Constraints */
  assignLpVariablePerPath ();
  setFlowDataRateConstraint (false /* Allow reduced flow rate */);
  setLinkCapacityConstraint ();

  /* Objective */
  setMaxDelayDistributionObjective ();

  return solveLpProblem ("Maximum Delay Metric");
}

void
LpSolver::assignLpVariablePerPath ()
{
  for (std::unique_ptr<Path> &path : m_paths)
    {
      lemon::Lp::Col dataRateOnPath = m_lpSolver.addCol ();
      path->setDataRateLpVar (dataRateOnPath);

      m_lpSolver.addRow (dataRateOnPath >= 0); // Ensure non-negative data rate assignment
    }
}

void
LpSolver::setFlowDataRateConstraint (bool allowReducedFlowRate)
{
  for (std::unique_ptr<Flow> &flow : *m_flows)
    {
      lemon::Lp::Expr flowDataRateExpression;

      for (Path *path : flow->getPaths ())
        {
          flowDataRateExpression += path->getDataRateLpVar ();
        }

      if (allowReducedFlowRate)
        m_lpSolver.addRow (flowDataRateExpression <= flow->getRequestedDataRate ());
      else
        m_lpSolver.addRow (flowDataRateExpression == flow->getAllocatedDataRate ());
    }
}

void
LpSolver::setLinkCapacityConstraint ()
{
  for (auto &linkTuple : m_links)
    {
      std::unique_ptr<Link> &link{linkTuple.second};

      lemon::Lp::Expr linkCapacityExpression;
      for (Path *path : link->getPaths ())
        {
          linkCapacityExpression += path->getDataRateLpVar ();
        }

      for (Path *ackPath : link->getAckPaths ())
        {
          // The below calculation assumes an ACK packet transmitted every 2 Data packets received
          linkCapacityExpression += (ackPath->getDataRateLpVar () * 0.0458);
        }

      m_lpSolver.addRow (linkCapacityExpression <= link->getCapacity ());
    }
}

void
LpSolver::setTotalNetworkFlowConstraint (double totalNetworkFlow)
{
  lemon::Lp::Expr totalNetworkFlowExpression;

  for (std::unique_ptr<Path> &path : m_paths)
    {
      totalNetworkFlowExpression += path->getDataRateLpVar ();
    }

  m_lpSolver.addRow (totalNetworkFlowExpression == totalNetworkFlow);
}

void
LpSolver::setMaxFlowObjective ()
{
  lemon::Lp::Expr maxFlowObjective;

  for (std::unique_ptr<Path> &path : m_paths)
    {
      maxFlowObjective += path->getDataRateLpVar ();
    }

  m_lpSolver.max (); // Set solver the find the solution with the largest value
  m_lpSolver.obj (maxFlowObjective);
}

void
LpSolver::setMinCostObjective ()
{
  lemon::Lp::Expr minCostObjective;

  for (std::unique_ptr<Path> &path : m_paths)
    minCostObjective += (path->getDataRateLpVar () * path->getCost ());

  m_lpSolver.min (); // Set solver the find the solution with the smallest value
  m_lpSolver.obj (minCostObjective);
}

void
LpSolver::setMaxDelayDistributionObjective ()
{
  lemon::Lp::Expr maxPathDelayObjective;

  for (const auto &flow : *m_flows)
    {
      // No calculation required when a flow is allocated nothing. If we do not
      // skip unallocated flows we would get an error due to a division by zero
      // when normalising the objective value.
      if (flow->getAllocatedDataRate () == 0)
        continue;

      lemon::Lp::Expr flowMetricValue;

      auto flowPaths = flow->getPaths ();

      auto lowestPathCost = getLowestPathCost (flowPaths);

      for (const auto &path : flowPaths)
        {
          auto pathCost = path->getCost ();
          auto pathMultiplier = double{1 / ((pathCost - lowestPathCost) + 1)};

          flowMetricValue += (pathMultiplier * path->getDataRateLpVar ());
        }

      // Normalise the path objective such that each flow can have a value between
      // 0 and 1

      // NOTE: This is not a direct representation of what we want because we do
      // not want to fix the flow values by those given by the MaxFlow solution,
      // similar to what we have done with the minimum cost solution where only
      // the global data rate has been fixed and not the individual flow
      // allocation rate. It will have to do because Linear programming does not
      // allow division.
      maxPathDelayObjective += (flowMetricValue / flow->getAllocatedDataRate ());
    }

  m_lpSolver.max (); // Maximise the objective value
  m_lpSolver.obj (maxPathDelayObjective);
}

std::pair<bool, double>
LpSolver::solveLpProblem (const std::string &optimisationProblem)
{
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now ();
  m_lpSolver.solvePrimalExact ();
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now ();

  auto duration{std::chrono::duration_cast<std::chrono::milliseconds> (end - start).count ()};
  m_timings.emplace (optimisationProblem, duration);

  auto optimalSolutionFound = (m_lpSolver.primalType () == lemon::Lp::OPTIMAL);
  auto objectiveValue = m_lpSolver.primal ();

  return std::make_pair (optimalSolutionFound, objectiveValue);
}

std::pair<bool, double>
LpSolver::FindMaxDelayMaxFlowLimit ()
{
  // Assign an LP variable per path and set the Flow Data Rate constraint
  for (const auto &flow : *m_flows)
    {
      lemon::Lp::Expr flowDataRateExpression;

      // Assign an LP data rate variable to each path
      auto flowPaths = flow->getPaths ();
      auto lowestPathCost = getLowestPathCost (flowPaths);

      for (auto &path : flowPaths)
        {
          lemon::Lp::Col dataRateOnPath = m_lpSolver.addCol ();
          path->setDataRateLpVar (dataRateOnPath);

          // Allow data rate assignments to the lowest cost paths only
          if (path->getCost () != lowestPathCost)
            m_lpSolver.addRow (dataRateOnPath == 0);
          else
            m_lpSolver.addRow (dataRateOnPath >= 0); // Ensure non-negative data rate assignment

          flowDataRateExpression += dataRateOnPath;
        }

      // The flow assigned data rate needs to be larger than zero BUT lower than
      // the flow's requested data rate.
      // The > 0 is not used because strict inequality is not supported by LP.
      // Answer: https://stackoverflow.com/questions/55936995/constraint-error-with-greater-than-operator
      m_lpSolver.addRow (flowDataRateExpression >= 0.000001);
      m_lpSolver.addRow (flowDataRateExpression <= flow->getRequestedDataRate ());
    }

  setLinkCapacityConstraint ();
  setMaxFlowObjective ();

  auto [solutionFound, objectiveValue] = solveLpProblem ("Maximum Flow");

  if (solutionFound)
    {
      for (std::unique_ptr<Flow> &flow : *m_flows) // Update the flow data rates
        {
          flow->calculateAllocatedDataRate (m_lpSolver);
        }
    }

  return std::make_pair (solutionFound, objectiveValue);
}
