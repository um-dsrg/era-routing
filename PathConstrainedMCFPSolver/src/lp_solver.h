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
  bool solve ();

private:
  void assignLpVariablePerPath ();
  void setFlowDataRateConstraint (bool allowReducedFlowRate);
  void setLinkCapacityConstraint ();

  void setMaxFlowObjective ();
  void setMinCostObjective ();

  bool solveMaxFlowProblem ();
  bool solveMinCostProblem ();

  linkContainer_t& m_links;
  pathContainer_t& m_paths;
  flowContainer_t& m_flows;

  lemon::GlpkLp m_lpSolver;
};

#endif /* SRC_LP_SOLVER_H_ */
