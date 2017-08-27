#ifndef GRAPH_MANAGER_H
#define GRAPH_MANAGER_H

#include <map>
#include <tinyxml2.h>

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
   */
  void ParseGraph (const std::string& lgfPath);

  void VerifyFlows ();
  /**
   *  \brief Finds the optimal solution
   */
  void FindOptimalSolution ();

  /**
   *  \brief Returns a bool that demonstrates whether the optimal solution was found or not
   *  \return bool
   */
  bool OptimalSolutionFound ();
  void AddLogsInXmlFile (tinyxml2::XMLDocument& xmlDoc);
private:
  // Linear Programming Functions /////////////////////////////////////////////
  /**
   *  \brief Add the Flows to the LP problem
   *
   *  Associate an LP variable with each link for each flow. The value of this variable is found
   *  by the optimal solution and it represents the fraction of flow X that will pass onto link Y.
   */
  void AddFlows ();
  /**
   *  \brief Adds the capacity constraint to the LP problem.
   *
   *  Adds the capacity constraint to the LP problem such that no link is used beyond
   *  its capacity.
   */
  void AddCapacityConstraint ();
  /**
   *  \brief Adds the balance constraint to the LP problem.
   *
   *  Adds the balance constraint to the LP problem. This condition will ensure that the flow's
   *  source node will transmit all the flow it has available, the destination node will receive
   *  all the data being transmitted and intermediate nodes do not hold any packets.
   */
  void AddBalanceConstraint ();
  /**
   *  \brief Adds the objective of the LP problem
   *
   *  Adds the LP objective. The current objective is to minimise the total network cost. The
   *  link cost is set equal to the link's delay and the network cost is calculated by multiplying
   *  the link cost with the amount of flow passing through that link.
   */
  void AddObjective ();

  // LP Solver ////////////////////////////////////////////////////////////////
  void SolveLpProblem ();
  lemon::Lp m_lpSolver;
  double m_duration;
  bool m_optimalSolutionFound; /*!< True if optimal solution found. False otherwise. */

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

  // XML Functionality ////////////////////////////////////////////////////////
  void LogDuration (tinyxml2::XMLDocument& xmlDoc);
  void LogOptimalSolution (tinyxml2::XMLDocument& xmlDoc);
  void LogIncomingFlow (tinyxml2::XMLDocument& xmlDoc);
  void LogNetworkTopology (tinyxml2::XMLDocument& xmlDoc);
  void LogNodeConfiguration (tinyxml2::XMLDocument& xmlDoc);

  tinyxml2::XMLElement* CreateLinkElement (tinyxml2::XMLDocument& xmlDoc,
                                           lemon::SmartDigraph::Arc& link);
};

#endif /* GRAPH_MANAGER_H */
