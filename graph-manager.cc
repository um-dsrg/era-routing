#include <chrono>

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
  // Add the flows
  AddFlows();

  // Add the constraints
  AddCapacityConstraint();
  AddBalanceConstraint();

  // Add the objective
  AddObjective();

  // Set the solver to find the solution with minimal cost
  m_lpSolver.min();

  // Solve the problem
  SolveLpProblem();
}

void
GraphManager::AddFlows ()
{
  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      for (lemon::SmartDigraph::ArcIt link(m_graph); link != lemon::INVALID; ++link)
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
  // Loop through all the available links on the graph and add the capacity constraint.
  for (lemon::SmartDigraph::ArcIt link(m_graph); link != lemon::INVALID; ++link)
    {
      lemon::Lp::Expr linkTotalCapacity;

      // Adding all the flow fractions that are passing on the link referred to by link.
      for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
        {
          linkTotalCapacity += m_optimalFlowRatio[std::make_pair(flow.id, link)];
        }

      // Adding the constraint in the LP problem
      m_lpSolver.addRow(linkTotalCapacity <= m_linkCapacity[link]);
    }
}

void
GraphManager::AddBalanceConstraint ()
{
  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows
    {
      // Looping through all the nodes
      for (lemon::SmartDigraph::NodeIt node(m_graph); node != lemon::INVALID; ++node)
        {
          lemon::Lp::Expr nodeBalanceExpression;

          // Add all outgoing flows from the node
          for (lemon::SmartDigraph::OutArcIt outgoingLink (m_graph, node);
               outgoingLink != lemon::INVALID; ++outgoingLink)
            {
              nodeBalanceExpression += m_optimalFlowRatio[std::make_pair(flow.id, outgoingLink)];
            }

          // Subtract all incoming flows from the node
          for (lemon::SmartDigraph::InArcIt incomingLink (m_graph, node);
               incomingLink != lemon::INVALID; ++incomingLink)
            {
              nodeBalanceExpression -= m_optimalFlowRatio[std::make_pair(flow.id, incomingLink)];
            }

          uint32_t nodeId = uint32_t (m_graph.id(node)); // Current node's ID

          if (flow.source == nodeId) // Source Node
            {
              m_lpSolver.addRow(nodeBalanceExpression == flow.dataRate);
            }
          else if (flow.destination == nodeId) // Sink Node
            {
              m_lpSolver.addRow(nodeBalanceExpression == -flow.dataRate);
            }
          else // Intermediate node
            {
              m_lpSolver.addRow(nodeBalanceExpression == 0);
            }
        }
    }
}

void
GraphManager::AddObjective ()
{
  lemon::Lp::Expr objective;

  //minimise the link cost * fraction of flow passing through it.

  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      for (lemon::SmartDigraph::ArcIt link(m_graph); link != lemon::INVALID; ++link)
        {
          // Retrieving the link delay (i.e. the cost) and multiplying it with the flow fraction
          // passing through that link. This is repeated for all Flows on all links.
          objective += (m_linkDelay[link] * m_optimalFlowRatio[std::make_pair(flow.id, link)]);
        }
    }

  // Set the objective
  m_lpSolver.obj (objective);
}

void
GraphManager::SolveLpProblem ()
{
  auto startTime = std::chrono::high_resolution_clock::now();
  m_lpSolver.solve();
  auto endTime = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> durationInMs = endTime - startTime;

  if (m_lpSolver.primalType() == lemon::Lp::OPTIMAL)
    {
#ifdef DEBUG
      std::cout << "Optimal Solution FOUND.\n"
                << "Solver took: " << durationInMs.count() << "ms" << std::endl;
#endif
    }
  else
    {
#ifdef DEBUG
      std::cout << "Optimal Solution NOT FOUND."
                << "Solver took: " << durationInMs.count() << "ms" << std::endl;
#endif
    }
}
