#ifndef LP_SOLVER_H
#define LP_SOLVER_H

#include <utility>

#include <lemon/lp.h>

#include "definitions.h"

class LpSolver
{
public:
  LpSolver (linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows);

  bool SolveProblem (const std::string& optimisationProblem);

  double GetLpColValue (lemon::Lp::Col lpCol) { return m_lpSolver.primal(lpCol); }
  const std::map<std::string, double>& GetTimings() { return m_timings; }
  const std::map<std::string, double>& GetObjectiveValues() { return m_objectiveValues; }

private:
  /* Problem solvers */
  bool MaxFlowMinCost ();
  bool MaxFlowFlowLimitedMinCost();
  bool MaxFlowMaxDelayMetric ();

  /* Problem definitions */
  std::pair<bool, double> solveMaxFlowProblem ();
  std::pair<bool, double> solveMinCostProblem (bool flowLimitedMinCost,
                                               double totalNetworkFlow = 0.0);
  std::pair<bool, double> solveMaxPathDelayProblem ();

  /* Constraints */
  void assignLpVariablePerPath ();
  void setFlowDataRateConstraint (bool allowReducedFlowRate);
  void setLinkCapacityConstraint ();
  void setTotalNetworkFlowConstraint (double totalNetworkFlow);

  /* Objectives */
  void setMaxFlowObjective ();
  void setMinCostObjective ();
  void setMaxPathDelayMetricObjective();

  /* Miscellaneous */
  std::pair<bool, double> FindMaxDelayMaxFlowLimit ();

  /* Lp Solver */
  std::pair<bool, double> solveLpProblem (const std::string& optimisationProblem);

  linkContainer_t& m_links;
  pathContainer_t& m_paths;
  flowContainer_t& m_flows;

  lemon::GlpkLp m_lpSolver;

  std::map<std::string, double> m_timings;
  std::map<std::string, double> m_objectiveValues;
};

#endif /* LP_SOLVER_H */
