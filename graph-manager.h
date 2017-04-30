#ifndef GRAPH_MANAGER_H
#define GRAPH_MANAGER_H

#include <map>

#include <lemon/smart_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/graph_to_eps.h>

#include <lemon/lp.h>

#include "flow-manager.h"

class GraphManager
{
public:
  GraphManager(const std::vector<FlowManager::Flow>* flows);

  /**
   *  \brief Parse the LGF file contents into a LEMON graph
   *  \param lgfPath The full path to the LGF file
   *  \return nothing
   */
  void ParseGraph (const std::string& lgfPath);

  // TODO: To IMPLEMENT AND COMMENT
  void FindOptimalSolution ();
private:
  // Linear Programming Functions /////////////////////////////////////////////
  /**
   *  \brief Add the Flows to the LP problem
   *
   *  Associate an LP variable with each link for each flow. The value of this variable is found
   *  by the optimal solution and it represents the fraction of flow X that will pass onto link Y.
   *
   *  \return nothing
   */
  void AddFlows ();
  void AddCapacityConstraint ();
  void AddBalanceConstraint ();
  void AddObjective ();

  lemon::Lp m_lpSolver;

  // Graph Related Variables //////////////////////////////////////////////////
  lemon::SmartDigraph m_graph;
  lemon::SmartDigraph::NodeMap<char> m_nodeType;
  lemon::SmartDigraph::ArcMap<double> m_linkCapacity;
  lemon::SmartDigraph::ArcMap<double> m_linkDelay;

  // Storing the node's coordinates
  lemon::SmartDigraph::NodeMap<lemon::dim2::Point<int>> m_nodeCoordinates;
  // Storing the node's shape
  lemon::SmartDigraph::NodeMap<int> m_nodeShape; /*!< CIRCLE = 0, SQUARE = 1, DIAMOND = 2 */
  // Storing the node's colour
  lemon::SmartDigraph::NodeMap<lemon::Color> m_nodeColour;

  // Flows ////////////////////////////////////////////////////////////////////
  const std::vector<FlowManager::Flow> * m_flows;

  // Key (<FlowID, Link>), Value (The fraction of flow represented by FlowID that will pass on
  // the link represented by Link.)
  std::map<std::pair<uint32_t, lemon::SmartDigraph::Arc>, lemon::Lp::Col> m_optimalFlowRatio;
};

#endif /* GRAPH_MANAGER_H */
