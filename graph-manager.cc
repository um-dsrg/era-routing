#include "graph-manager.h"

GraphManager::GraphManager(const std::vector<FlowManager::Flow>* flows): m_nodeType(m_graph),
                                                                         m_linkCapacity(m_graph),
                                                                         m_linkDelay(m_graph),
                                                                         m_nodeCoordinates(m_graph),
                                                                         m_nodeShape(m_graph),
                                                                         m_nodeColour(m_graph),
                                                                         m_flows(flows)
{}

void
GraphManager::ParseGraph(const std::string &lgfPath)
{
  try
    {
      lemon::digraphReader(m_graph, lgfPath). // Read the graph
        nodeMap("coordinates", m_nodeCoordinates).
        nodeMap("type", m_nodeType).
        arcMap("capacity", m_linkCapacity).
        arcMap("delay", m_linkDelay).
        run ();

#ifdef DEBUG
      std::cout << "Graph parsed successfully" << std::endl;
#endif
    }
  catch (lemon::Exception& e)
    {
      std::cerr << "Error parsing the LGF graph\n"
                << "LGF Location: "<< lgfPath << "\n"
                << "Error: " << e.what() << std::endl;
      throw;
    }
}

void
GraphManager::FindOptimalSolution()
{
  // TODO: TO IMPLEMENT
}

void
GraphManager::AddFlows ()
{
  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      for (lemon::SmartDigraph::ArcIt link = lemon::SmartDigraph::ArcIt(m_graph);
           link != lemon::INVALID;
           ++link)
        {
          // Add an LP variable that will store the fraction of the flow(represented by flow.id)
          // that will pass on the Link represented by link.
          lemon::Lp::Col fractionOfFlow = m_lpSolver.addCol();

          // Store the fraction flow variable in the map so we can access it once an optimal
          // solution is found
          m_optimalFlowRatio[std::make_pair(flow.id, link)] = fractionOfFlow;

          // Add constraint that a flow must be greater than or equal to 0. I.e. no -ve values are
          // allowed.
          m_lpSolver.addRow(fractionOfFlow >= 0);
        }
    }
}

void
GraphManager::AddCapacityConstraint ()
{

}

void
GraphManager::AddBalanceConstraint ()
{

}

void
GraphManager::AddObjective ()
{

}

// void
// CommodityUtilities::AddLpCapacityConstraint()
// {
//   // Loop through all the arcs
//   for (l_ArcIt arc = m_graphUtilities.GetArcIterator();
//        arc != lemon::INVALID; ++arc)
//     {
//       l_LpExpr capacitySummationPerArc;

//       // Loop through all the commodities
//       for (CommoditySize_t commodityId = 0; commodityId < m_commodities.size(); commodityId++)
//         {
//           // Add the flow of the particular commodity on the Arc defined by arc
//           capacitySummationPerArc += m_flowPerArcPerCommodity[std::make_pair (commodityId, arc)];
//         }

//       // Add the capacity constraint
//       m_lpSolver.addRow (capacitySummationPerArc <= m_graphUtilities.GetArcCapacity(arc));
//     }
// }

// void
// CommodityUtilities::AddLpBalanceConstraint()
// {
//   // Loop through all the commodities
//   int commodityId = 0;
//   for (CommodityProperties& commodity : m_commodities)
//     {
//       // Loop through all the nodes
//       for (l_NodeIt currentNode = m_graphUtilities.GetNodeIterator();
//            currentNode != lemon::INVALID; ++currentNode)
//         {
//           l_LpExpr nodeBalanceConstraint;

//           // Add the outgoing flows going out of the node
//           for (l_OutArcIt outgoingArc = m_graphUtilities.GetOutgoingArcsIterator(currentNode);
//                outgoingArc != lemon::INVALID; ++outgoingArc)
//             {
//               nodeBalanceConstraint += m_flowPerArcPerCommodity[std::make_pair(commodityId, outgoingArc)];
//             }

//           // Subtract the flows going in the node
//           for (l_InArcIt incomingArc = m_graphUtilities.GetIncomingArcsIterator(currentNode);
//                incomingArc != lemon::INVALID; ++incomingArc)
//             {
//               nodeBalanceConstraint -= m_flowPerArcPerCommodity[std::make_pair(commodityId, incomingArc)];
//             }

//           if (commodity.GetSourceId() == m_graphUtilities.GetNodeId(currentNode))
//             {
//               // Source Node
//               double actualDataRate = CalculateActualCommodityDataRate(commodity);
//               NS_LOG_INFO("The actual data rate for commodity " << commodityId << " is "
//                           << actualDataRate << "Mbps from (" << commodity.GetDataRate() << "Mbps)");

//               m_lpSolver.addRow(nodeBalanceConstraint == CalculateActualCommodityDataRate(commodity));
//             }
//           else if (commodity.GetSinkId() == m_graphUtilities.GetNodeId(currentNode))
//             {
//               // Sink Node
//               m_lpSolver.addRow(nodeBalanceConstraint == -CalculateActualCommodityDataRate(commodity));
//             }
//           else
//             {
//               // Intermediate node
//               m_lpSolver.addRow(nodeBalanceConstraint == 0);
//             }
//         }

//       commodityId++;
//     }
// }

// void
// CommodityUtilities::AddLpObjective()
// {
//   l_LpExpr objective;

//   // Loop through all commodities
//   for (CommoditySize_t commodityId = 0; commodityId < m_commodities.size(); commodityId++)
//     {
//       for (l_ArcIt arc = m_graphUtilities.GetArcIterator();
//            arc != lemon::INVALID; ++arc)
//         {
//           // Get the flow of commodity k on arc i,j and multiplying it
//           // with the link cost.
//           objective += (m_graphUtilities.GetArcCost(arc) *
//                         m_flowPerArcPerCommodity[std::make_pair(commodityId, arc)]);
//         }
//     }

//   // We need to find the solution with minimal cost
//   m_lpSolver.min ();

//   // Define objective
//   m_lpSolver.obj (objective);
// }
