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
  GraphManager(std::vector<FlowManager::Flow>* flows);

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
   * @brief FindMaximumFlowSolution
   *
   * Finds the maximum routes. This does not necessarily mean that it is the solution with
   * minimum cost. The FindMinimumCostSolution function will be used after this to find the
   * solution with minimum cost.
   */
  void FindMaximumFlowSolution ();

  /**
   * @brief FindMinimumCostSolution
   *
   * Given the flow values from the FindMaximumFlowSolution function, this function will find
   * the routes that have the smallest cost.
   */
  void FindMinimumCostSolution ();
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
   *
   * \param allowReducedFlowRate When set to true the balance constraint will allow flows to receive
   * 														 less than what they have requested.
   */
  void AddBalanceConstraint (bool allowReducedFlowRate);
  /**
   *  \brief Adds the objective of the LP problem when finding the maximum flows
   *
   *  Adds the LP objective. The objective is to maximise the total network usage. Flows can have
   *  smaller data rates than what they requested.
   */
  void AddMaximumFlowObjective ();
  /**
   *  \brief Adds the objective of the LP problem when finding the minimum cost
   *
   *  Adds the LP objective. The objective is to minimise the total network cost. The
   *  link cost is set equal to the link's delay and the network cost is calculated by multiplying
   *  the link cost with the amount of flow passing through that link.
   */
  void AddMinimumCostObjective ();

  /**
   * @brief UpdateFlowDataRates will update the flow data rates based on the maximal LP solution
   *
   * Once the optimal maximisation has complete, the requested data rate and the given data rate
   * might not be the same. Therefore this function will update the flows with the data rate given
   * by the maximal optimal solution.
   */
  void UpdateFlowDataRates ();

  // LP Solver ////////////////////////////////////////////////////////////////
  /**
   * @brief SolveLpProblem
   *
   * Solves the LP problem and returns the time taken by the solver.
   *
   * @return The time taken in ms to solve the Lp problem.
   */
  double SolveLpProblem ();

  lemon::Lp m_lpSolver;
  double m_durationMaximumFlow; /*!< Stores the time taken to find the solution with maximum flow. */
  double m_durationMinimumCost; /*!< Stores the time taken to find the solution with minimal cost. */
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
  std::vector<FlowManager::Flow> * m_flows;

  struct FlowDetails
  {
    uint32_t id;
    double requestedDataRate;
    double receivedDataRate;

    FlowDetails (uint32_t id, double requestedDataRate, double receivedDataRate) :
      id (id), requestedDataRate (requestedDataRate), receivedDataRate (receivedDataRate)
    {}
  };
  std::vector<FlowDetails> m_modifiedFlows;

  // Key (<FlowID, Link>), Value (The fraction of flow represented by FlowID that will pass on
  // the link represented by Link.)
  std::map<std::pair<uint32_t, lemon::SmartDigraph::Arc>, lemon::Lp::Col> m_optimalFlowRatio;

  // XML Functionality ////////////////////////////////////////////////////////
  void LogDuration (tinyxml2::XMLDocument& xmlDoc);
  void LogOptimalSolution (tinyxml2::XMLDocument& xmlDoc);
  void LogIncomingFlow (tinyxml2::XMLDocument& xmlDoc);
  void LogNetworkTopology (tinyxml2::XMLDocument& xmlDoc);
  void LogNodeConfiguration (tinyxml2::XMLDocument& xmlDoc);
  void LogFlowDataRateUpdates (tinyxml2::XMLDocument& xmlDoc);

  tinyxml2::XMLElement* CreateLinkElement (tinyxml2::XMLDocument& xmlDoc,
                                           lemon::SmartDigraph::Arc& link);
};

#endif /* GRAPH_MANAGER_H */
