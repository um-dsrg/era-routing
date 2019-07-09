/*
 * lp_solver.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_LP_SOLVER_H_
#define SRC_LP_SOLVER_H_

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

private:
  enum class Problem { MaxFlow, MinCost, MaxDelayMetric };

  void assignLpVariablePerPath ();
  void setFlowDataRateConstraint (bool allowReducedFlowRate);
  void setLinkCapacityConstraint ();

  void setMaxFlowObjective ();
  void setMinCostObjective ();
  void setMaxPathDelayMetricObjective();

  bool solveLpProblem (Problem problem);
  bool solveMaxFlowProblem ();
  bool solveMinCostProblem ();
  bool solveMaxPathDelayProblem ();

  linkContainer_t& m_links;
  pathContainer_t& m_paths;
  flowContainer_t& m_flows;

  double m_maxFlowDurationMs = 0.0;
  double m_minCostDurationMs = 0.0;
  double m_maxDelayDurationMs = 0.0;

  lemon::GlpkLp m_lpSolver;
};

#endif /* SRC_LP_SOLVER_H_ */
