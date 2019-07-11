#ifndef LP_SOLVER_H
#define LP_SOLVER_H

#include <utility>

#include <lemon/lp.h>

#include "definitions.h"

class LpSolver
{
public:
  LpSolver (linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows);

  double getLpColValue (lemon::Lp::Col lpCol) { return m_lpSolver.primal(lpCol); }

  double getMaxFlowDuration () const { return m_maxFlowDurationMs; }
  double getMinCostDuration () const { return m_minCostDurationMs; }
  double getMaxDelayDuration () const { return m_maxDelayDurationMs; }

  bool solve ();
  bool solveMaxFlowMinCost ();

  bool findMaxDelayMaxFlowLimit ();

private:
  enum class Problem { MaxFlow, MinCost, MaxDelayMetric };

  void assignLpVariablePerPath ();
  void setFlowDataRateConstraint ();
  void setLinkCapacityConstraint ();
  void setTotalNetworkFlowConstraint (double totalNetworkFlow);

  void setMaxFlowObjective ();
  void setMinCostObjective ();
  void setMaxPathDelayMetricObjective();

  std::pair<bool, double> solveLpProblem (Problem problem);

  std::pair<bool, double> solveMaxFlowProblem ();
  std::pair<bool, double> solveMinCostProblem (double totalNetworkFlow);
  bool solveMaxPathDelayProblem ();

  linkContainer_t& m_links;
  pathContainer_t& m_paths;
  flowContainer_t& m_flows;

  double m_maxFlowDurationMs = 0.0;
  double m_minCostDurationMs = 0.0;
  double m_maxDelayDurationMs = 0.0;

  lemon::GlpkLp m_lpSolver;
};

#endif /* LP_SOLVER_H */
