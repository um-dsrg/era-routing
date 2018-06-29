#ifndef GRAPH_MANAGER_H
#define GRAPH_MANAGER_H

#include <map>
#include <tinyxml2.h>

#include <lemon/smart_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/graph_to_eps.h>

#include <lemon/cplex.h>
#include <lemon/lp.h>

#include "flow-manager.h"

class GraphManager
{
public:
  GraphManager (std::vector<FlowManager::Flow>* flows);

  /**
   *  \brief Parse the LGF file contents into a LEMON graph
   *  \param lgfPath The full path to the LGF file
   */
  void ParseGraph (const std::string& lgfPath);

  void VerifyFlows ();
  /**
   *  \brief Finds the optimal solution
   *  \param solverConfig A string that determines which solver to use. A
   *  description of the available parameters is found in lp-solver.cc
   */
  void FindOptimalSolution (std::string& solverConfig);

  /**
   *  \brief Returns a bool that demonstrates whether the optimal solution was found or not
   *  \return bool
   */
  bool OptimalSolutionFound ();

  /**
   * Generate the routes for the ACK flows using the Dijkstra algorithm
   */
  void GenerateAckRoutes();

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
   *      less than what they have requested.
   */
  void AddBalanceConstraint (bool allowReducedFlowRate);
  /**
   * @brief Adds a constraint to avoid loops at source/destination.
   *
   * Adds a constraint such that a source node cannot be receiving any of the
   * flow that it is transmitting, and a destination node cannot be
   * transmitting any of the flow that it is receiving.
   *
   * This constraint is required because we have relaxed the amount of flow a
   * node can receive.
   */
  void AddNoLoopConstraint ();
  /**
   * @brief      Adds a constraint that will make sure that all flows transmit something.
   *
   * This constraint will make sure that all the flows will transmit something.
   * Without this constraint, Acknowledgment flows may have a flow of 0 while their data
   * counterpart can still transmit. This scenario will cause issues with the ns3 simulation.
   * Therefore, to solve this in the most simplistic way, a constraint was added that a flow
   * has to transmit something.
   */
  void AddNoZeroFlowConstraint ();
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

  // Timing ///////////////////////////////////////////////////////////////////
  struct Timing
  {
    // All values stored are in ms.
    double cpuTime;
    double realTime;

    Timing () : cpuTime (0.0), realTime (0.0)
    {}
  };

  Timing m_maxFlowTiming; /*!< Stores the time taken to find the solution with maximum flow. */
  Timing m_minCostTiming; /*!< Stores the time taken to find the solution with minimal cost. */

  // LP Solver ////////////////////////////////////////////////////////////////
  /**
   * @brief SolveLpProblem
   *
   * Solves the LP problem and returns the time taken by the solver.
   *
   * @return The time taken in ms to solve the Lp problem.
   */
  void SolveLpProblem (Timing& timing);

  lemon::Lp m_lpSolver;
  bool m_optimalSolutionFound; /*!< True if optimal solution found. False otherwise. */

  // Graph Related Variables //////////////////////////////////////////////////
  typedef lemon::SmartDigraph LGraph;
  typedef LGraph::Node LNode;
  typedef LGraph::Arc LArc;
  typedef LGraph::ArcMap<double> LArcDelay;

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

  // Key: <FlowID, Link>
  // Value: The fraction of flow represented by FlowID that will pass on the
  //        link represented by Link.
  std::map<std::pair<uint32_t,
                     lemon::SmartDigraph::Arc>,
           lemon::Lp::Col> m_optimalFlowRatio;

  // ACK Flows //////////////////////////////////////////////////////////////
  // Key: FlowId
  // Value: List of Link Ids the flow defined by FlowId passes through
  std::map<uint32_t, std::vector<uint32_t>> m_ackRoutes;

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
