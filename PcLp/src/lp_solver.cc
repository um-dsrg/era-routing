#include <chrono>
#include <algorithm>

#include "lp_solver.h"

/**
 * @brief Retrieve the lowest path cost from a given set of paths
 *
 * @param flowPaths The set of paths related to a flow
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

LpSolver::LpSolver (linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows) :
  m_links (links), m_paths(paths), m_flows(flows)
{}


bool
LpSolver::SolveProblem(const std::string& optimisationProblem)
{
  auto optimalSolutionFound {false};

  if (optimisationProblem == "MaxFlow_MinCost")
  {
    std::cout << "Solving the Maximum Flow Minimum Cost problem..." << std::endl;
    optimalSolutionFound = solveMaxFlowMinCost();
  }
  else if (optimisationProblem == "MaxFlow_FlowLimitedMinCost")
  {
    std::cout << "Solving the Maximum Flow Minimum Cost problem with each flow's assigned data rate"
              << "set by Maximum Flow solution..." << std::endl;
  }
  else
  {
    throw std::runtime_error(optimisationProblem + " is not supported");
  }

  return optimalSolutionFound;
}

bool
LpSolver::solveMaxFlowMinCost ()
{
  auto [maxFlowoptimalSolutionFound, maxNetworkFlow] = solveMaxFlowProblem();

  if (!maxFlowoptimalSolutionFound) return false;

  m_lpSolver.clear(); // Reset the LP solver

  auto [minCostOptimalSolnFound,
        minNetworkCost] = solveMinCostProblem(false /* Flow Limited Minimum Cost */,
                                              maxNetworkFlow);

  for (std::unique_ptr<Flow>& flow: m_flows)
    flow->calculateAllocatedDataRate(m_lpSolver);

  return minCostOptimalSolnFound;
}

bool
LpSolver::solveMaxFlowFlowLimitedMinCost ()
{
  auto [maxFlowoptimalSolutionFound, maxNetworkFlow] = solveMaxFlowProblem();

  if (!maxFlowoptimalSolutionFound) return false;

  m_lpSolver.clear(); // Reset the LP solver

  for (std::unique_ptr<Flow>& flow: m_flows)
    flow->calculateAllocatedDataRate(m_lpSolver);

  auto [minCostOptimalSolnFound,
        minNetworkCost] = solveMinCostProblem(true /*Flow limited Minimum Cost*/);

  return minCostOptimalSolnFound;
}

std::pair<bool, double>
LpSolver::solveMaxFlowProblem ()
{
  /* Variable assignments + Constraints */
  assignLpVariablePerPath();
  setFlowDataRateConstraint(true /* Allow reduced Flow Rate */);
  setLinkCapacityConstraint();

  /* Objective */
  setMaxFlowObjective();

  return solveLpProblem(Problem::MaxFlow);
}

std::pair<bool, double>
LpSolver::solveMinCostProblem (bool flowLimitedMinCost, double totalNetworkFlow)
{
  /* Variable assignments + Constraints */
  assignLpVariablePerPath();
  setFlowDataRateConstraint(!flowLimitedMinCost /* Allow reduced flow rate */);
  setLinkCapacityConstraint();

  if (!flowLimitedMinCost)
    setTotalNetworkFlowConstraint(totalNetworkFlow);

  /* Objective */
  setMinCostObjective();

  return solveLpProblem(Problem::MinCost);
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
      {
        flowDataRateExpression += path->getDataRateLpVar();
      }

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
LpSolver::setTotalNetworkFlowConstraint (double totalNetworkFlow)
{
  lemon::Lp::Expr totalNetworkFlowExpression;

  for (std::unique_ptr<Path>& path: m_paths)
  {
    totalNetworkFlowExpression += path->getDataRateLpVar();
  }

  m_lpSolver.addRow(totalNetworkFlowExpression == totalNetworkFlow);
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

std::pair<bool, double>
LpSolver::solveLpProblem (Problem problem)
{
  std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
  m_lpSolver.solvePrimalExact();
  std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();

  // FIXME: Update the below to use a dictionary as well!
  switch(problem)
  {
    case Problem::MaxFlow:
      m_maxFlowDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>
                            (end - start).count();
      break;
    case Problem::MinCost:
      m_minCostDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>
                            (end - start).count();
      break;
    case Problem::MaxDelayMetric:
      m_maxDelayDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>
                             (end - start).count();
      break;
  }

  auto optimalSolutionFound = (m_lpSolver.primalType() == lemon::Lp::OPTIMAL);
  auto objectiveValue = m_lpSolver.primal();

  return std::make_pair(optimalSolutionFound, objectiveValue);
}

void
LpSolver::setMaxPathDelayMetricObjective()
{
  lemon::Lp::Expr maxPathDelayObjective;

  for (const auto& flow: m_flows)
  {
    // No calculation required when a flow is allocated nothing. If we do not
    // skip unallocated flows we would get an error due to a division by zero
    // when normalising the objective value.
    if (flow->getAllocatedDataRate() == 0)
      continue;

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
LpSolver::solveMaxPathDelayProblem ()
{
  assignLpVariablePerPath();
  // FIXME: The below boolean flag might require tweaking
  setFlowDataRateConstraint(true /* Allow reduced flow rate */);
  setLinkCapacityConstraint();
  setMaxPathDelayMetricObjective();

  auto [solutionFound, metricValue] = solveLpProblem(Problem::MaxDelayMetric);

  return solutionFound;
}

bool
LpSolver::findMaxDelayMaxFlowLimit ()
{
  // Assign an LP variable per path and set the Flow Data Rate constraint
  for (const auto& flow: m_flows)
  {
    lemon::Lp::Expr flowDataRateExpression;

    // Assign an LP data rate variable to each path
    auto flowPaths = flow->getPaths();
    auto lowestPathCost = getLowestPathCost(flowPaths);

    for (auto& path: flowPaths)
    {
      lemon::Lp::Col dataRateOnPath = m_lpSolver.addCol();
      path->setDataRateLpVar(dataRateOnPath);

      // Allow data rate assignments to the lowest cost paths only
      if (path->getCost() != lowestPathCost)
        m_lpSolver.addRow(dataRateOnPath == 0);
      else
        m_lpSolver.addRow(dataRateOnPath >= 0); // Ensure non-negative data rate assignment

      flowDataRateExpression += dataRateOnPath;
    }

    // The flow assigned data rate needs to be larger than zero BUT lower than
    // the flow's requested data rate.
    // The > 0 is not used because strict inequality is not supported by LP.
    // Answer: https://stackoverflow.com/questions/55936995/constraint-error-with-greater-than-operator
    m_lpSolver.addRow(flowDataRateExpression >= 0.000001);
    m_lpSolver.addRow(flowDataRateExpression <= flow->getRequestedDataRate());
  }

  setLinkCapacityConstraint();
  setMaxFlowObjective();

  auto [solutionFound, objectiveValue] = solveLpProblem(Problem::MaxFlow);

  if (solutionFound)
  {
    for (std::unique_ptr<Flow>& flow: m_flows) // Update the flow data rates
      flow->calculateAllocatedDataRate(m_lpSolver);
  }

  return solutionFound;
}
