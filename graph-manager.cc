#include <chrono>
#include <set>
#include <assert.h>
#include <ctime>

#include "graph-manager.h"
#include "xml-utilities.h"

GraphManager::GraphManager (std::vector<FlowManager::Flow>* flows):
  m_optimalSolutionFound (false),
  m_nodeType (m_graph),
  m_linkCapacity (m_graph),
  m_linkDelay (m_graph),
  m_nodeCoordinates (m_graph),
  m_nodeShape (m_graph),
  m_nodeColour (m_graph),
  m_flows (flows)
{}

void
GraphManager::ParseGraph (const std::string &lgfPath)
{
  try
    {
      lemon::digraphReader (m_graph, lgfPath). // Read the graph
      nodeMap ("coordinates", m_nodeCoordinates).
      nodeMap ("type", m_nodeType).
      arcMap ("capacity", m_linkCapacity).
      arcMap ("delay", m_linkDelay).
      run ();

#ifdef DEBUG
      std::cout << "Graph parsed successfully" << std::endl;
#endif
    }
  catch (lemon::Exception& e)
    {
      std::cerr << "Error parsing the LGF graph\n"
                << "LGF Location: " << lgfPath << "\n"
                << "Error: " << e.what() << std::endl;
      throw;
    }
}

void
GraphManager::VerifyFlows()
{
  try
    {
      for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
        {
          // Checking that the source is valid
          if (m_graph.valid (m_graph.nodeFromId (flow.source)) != true)
            {
              std::string errorMessage ("Flow " + std::to_string (flow.id) + " has an invalid source"
                                        " node with id " + std::to_string (flow.source));
              throw std::invalid_argument (errorMessage.c_str());
            }

          // Checking that the destination is valid
          if (m_graph.valid (m_graph.nodeFromId (flow.destination)) != true)
            {
              std::string errorMessage ("Flow " + std::to_string (flow.id) + " has an invalid"
                                        " destination node with id "
                                        + std::to_string (flow.destination));
              throw std::invalid_argument (errorMessage.c_str());
            }
        }
    }
  catch (std::invalid_argument& e)
    {
      std::cerr << e.what() << std::endl;
      throw;
    }

  // Throw an exception if something is not right.
}

void
GraphManager::FindOptimalSolution (std::string& solverConfig)
{
  try
    {
      if (solverConfig == "mf" || solverConfig == "mfmc")
        {
          // Find the maximum flows that can be passed through the network.
          FindMaximumFlowSolution ();
          if (!m_optimalSolutionFound) throw std::runtime_error ("Maximal solution not found");

          UpdateFlowDataRates (); // Update the flow data rates based on the Maximal flow solution.
        }

      if (solverConfig == "mc" || solverConfig == "mfmc")
        {
          m_lpSolver.clear (); // Resetting the LP Solver.
          // Find the minimum network cost to route the flows given from the maximum flow solutions.
          FindMinimumCostSolution ();
          if (!m_optimalSolutionFound) throw std::runtime_error ("Minimal cost solution not found");
        }
    }
  catch (std::runtime_error& e)
    {
      std::cerr << e.what () << std::endl;
      throw;
    }
}

bool
GraphManager::OptimalSolutionFound()
{
  return m_optimalSolutionFound;
}

void
GraphManager::AddLogsInXmlFile (tinyxml2::XMLDocument& xmlDoc)
{
  LogDuration (xmlDoc);
  // Log the solution and other relevant stuff only if an optimal solution was found.
  if (m_optimalSolutionFound)
    {
      LogOptimalSolution (xmlDoc);
      LogIncomingFlow (xmlDoc);
      LogNetworkTopology (xmlDoc);
      LogNodeConfiguration (xmlDoc);
      LogFlowDataRateUpdates (xmlDoc);
    }
}

void
GraphManager::FindMaximumFlowSolution ()
{
  // Add the flows
  AddFlows ();

  // Add Capacity Constraint
  AddCapacityConstraint ();

  // The balance constraint when finding the maximal solution will allow flows to receive
  // data rates smaller than what they have requested.
  AddBalanceConstraint (true);

  // Add a constraint to avoid any data going back to the transmitter or from
  // the receiver.
  AddNoLoopConstraint();

  // Add the maximum flow objective
  AddMaximumFlowObjective ();

  // Set the solver to find the solution with the maximum value
  m_lpSolver.max ();

  // Find a solution to the LP problem
  SolveLpProblem (m_maxFlowTiming);
}

void
GraphManager::FindMinimumCostSolution ()
{
  // Add the flows
  AddFlows();

  // Add Capacity Constraint
  AddCapacityConstraint();

  // The balance constraint when finding the minmal cost solution will not allow flows to
  // receive data rates smaller than what they have requested.
  AddBalanceConstraint (false);

  // Add the objective
  AddMinimumCostObjective ();

  // Set the solver to find the solution with minimal cost
  m_lpSolver.min ();

  // Solve the problem
  SolveLpProblem (m_minCostTiming);
}

void
GraphManager::AddFlows ()
{
  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      for (lemon::SmartDigraph::ArcIt link (m_graph); link != lemon::INVALID; ++link)
        {
          // Add an LP variable that will store the fraction of the flow(represented by flow.id)
          // that will pass on the Link represented by link.
          lemon::Lp::Col fractionOfFlow = m_lpSolver.addCol();

          // Store the fraction flow variable in the map so we can access it once an optimal
          // solution is found
          m_optimalFlowRatio[std::make_pair (flow.id, link)] = fractionOfFlow;

          // Add constraint that a flow must be greater than or equal to 0. I.e. no -ve values are
          // allowed.
          m_lpSolver.addRow (fractionOfFlow >= 0);
        }
    }
}

void
GraphManager::AddCapacityConstraint ()
{
  // Loop through all the available links on the graph and add the capacity constraint.
  for (lemon::SmartDigraph::ArcIt link (m_graph); link != lemon::INVALID; ++link)
    {
      lemon::Lp::Expr linkTotalCapacity;

      // Adding all the flow fractions that are passing on the link referred to by link.
      for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
        {
          linkTotalCapacity += m_optimalFlowRatio[std::make_pair (flow.id, link)];
        }

      // Adding the constraint in the LP problem
      m_lpSolver.addRow (linkTotalCapacity <= m_linkCapacity[link]);
    }
}

void
GraphManager::AddBalanceConstraint (bool allowReducedFlowRate)
{
  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows
    {
      // Looping through all the nodes
      for (lemon::SmartDigraph::NodeIt node (m_graph); node != lemon::INVALID; ++node)
        {
          lemon::Lp::Expr nodeBalanceExpression;

          // Add all outgoing flows from the node
          for (lemon::SmartDigraph::OutArcIt outgoingLink (m_graph, node);
               outgoingLink != lemon::INVALID; ++outgoingLink)
            {
              nodeBalanceExpression += m_optimalFlowRatio[std::make_pair (flow.id, outgoingLink)];
            }

          // Subtract all incoming flows from the node
          for (lemon::SmartDigraph::InArcIt incomingLink (m_graph, node);
               incomingLink != lemon::INVALID; ++incomingLink)
            {
              nodeBalanceExpression -= m_optimalFlowRatio[std::make_pair (flow.id, incomingLink)];
            }

          uint32_t nodeId = uint32_t (m_graph.id (node)); // Current node's ID

          if (flow.source == nodeId) // Source Node
            {
              if (allowReducedFlowRate)
                m_lpSolver.addRow (nodeBalanceExpression <= flow.dataRate);
              else
                m_lpSolver.addRow (nodeBalanceExpression == flow.dataRate);
            }
          else if (flow.destination == nodeId) // Sink Node
            {
              if (allowReducedFlowRate)
                m_lpSolver.addRow (nodeBalanceExpression >= -flow.dataRate);
              else
                m_lpSolver.addRow (nodeBalanceExpression == -flow.dataRate);
            }
          else // Intermediate node
            {
              m_lpSolver.addRow (nodeBalanceExpression == 0);
            }
        }
    }
}

void
GraphManager::AddNoLoopConstraint ()
{
  for (const FlowManager::Flow& flow : *m_flows) // Loop through all the flows
    {
      // Constraining the Source Node
      lemon::Lp::Expr totalIncomingFlow;
      lemon::SmartDigraph::Node srcNode = m_graph.nodeFromId (flow.source);

      // Add all the incoming data rate for the current flow at its source node
      // and make sure it equals 0
      for (lemon::SmartDigraph::InArcIt incomingLink (m_graph, srcNode);
           incomingLink != lemon::INVALID; ++incomingLink)
        {
          totalIncomingFlow += m_optimalFlowRatio[std::make_pair (flow.id, incomingLink)];
        }

      m_lpSolver.addRow (totalIncomingFlow == 0);

      // Constraining the Destination Node
      lemon::Lp::Expr totalOutgoingFlow;
      lemon::SmartDigraph::Node dstNode = m_graph.nodeFromId (flow.destination);

      // Add all the outgoing data rate for the current flow at its destination
      // node and make sure it equals 0
      for (lemon::SmartDigraph::OutArcIt outgoingLink (m_graph, dstNode);
           outgoingLink != lemon::INVALID; ++outgoingLink)
        {
          totalOutgoingFlow += m_optimalFlowRatio[std::make_pair (flow.id, outgoingLink)];
        }

      m_lpSolver.addRow (totalOutgoingFlow == 0);
    }
}

void
GraphManager::AddMaximumFlowObjective ()
{
  lemon::Lp::Expr objective;

  for (const FlowManager::Flow& flow : *m_flows)
    {
      // Get the flow's source node
      lemon::SmartDigraph::Node sourceNode = m_graph.nodeFromId (flow.source);

      // Loop through all the source node's outgoing links
      for (lemon::SmartDigraph::OutArcIt outgoingLink (m_graph, sourceNode);
           outgoingLink != lemon::INVALID; ++outgoingLink)
        {
          objective += m_optimalFlowRatio[std::make_pair (flow.id, outgoingLink)];
        }
    }

  // Set the objective
  m_lpSolver.obj (objective);
}

void
GraphManager::AddMinimumCostObjective ()
{
  lemon::Lp::Expr objective;

  //minimise the link cost * fraction of flow passing through it.

  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      for (lemon::SmartDigraph::ArcIt link (m_graph); link != lemon::INVALID; ++link)
        {
          // Retrieving the link delay (i.e. the cost) and multiplying it with the flow fraction
          // passing through that link. This is repeated for all Flows on all links.
          objective += (m_linkDelay[link] * m_optimalFlowRatio[std::make_pair (flow.id, link)]);
        }
    }

  // Set the objective
  m_lpSolver.obj (objective);
}

void
GraphManager::UpdateFlowDataRates ()
{
  for (FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      // Get the source node.
      lemon::SmartDigraph::Node sourceNode = m_graph.nodeFromId (flow.source);

      double flowAllocatedDataRate (0.0);

      // Looping through all the outgoing links of the source node and adding
      // the values from the solver's solution.
      for (lemon::SmartDigraph::OutArcIt outgoingLink (m_graph, sourceNode);
           outgoingLink != lemon::INVALID; ++outgoingLink)
        {
          flowAllocatedDataRate +=
            m_lpSolver.primal (m_optimalFlowRatio[std::make_pair (flow.id, outgoingLink)]);
        }

#ifdef DEBUG
      std::cout << "Flow ID: " << flow.id << " Requested flow rate: " <<
                flow.dataRate << std::endl;
      std::cout << "Flow ID: " << flow.id << " Received flow rate: " <<
                flowAllocatedDataRate << std::endl;
#endif

      // If the flow data rate was modified. We take note such that we can add it in the
      // log file.
      if (flow.dataRate != flowAllocatedDataRate)
        {
          m_modifiedFlows.push_back (FlowDetails (flow.id, flow.dataRate,
                                                  flowAllocatedDataRate));
          // Update the flow's data rate to that allocated by the solver
          flow.dataRate = flowAllocatedDataRate;
        }
    }
}

void
GraphManager::SolveLpProblem (Timing& timing)
{
  auto startTime = std::chrono::high_resolution_clock::now(); // Starting wallclock
  std::clock_t cpuStart = std::clock(); // Starting the CPU clock
  m_lpSolver.solve();
  std::clock_t cpuEnd = std::clock(); // Stopping the CPU clock
  auto endTime = std::chrono::high_resolution_clock::now(); // Stopping wallclock

  // CPU Timing
  timing.cpuTime = 1000.0 * (cpuEnd - cpuStart) / CLOCKS_PER_SEC;

  // Real time timing (Wall clock time)
  std::chrono::duration<double, std::milli> durationInMs = endTime - startTime;
  timing.realTime = durationInMs.count();

  if (m_lpSolver.primalType() == lemon::Lp::OPTIMAL)
    {
      m_optimalSolutionFound = true;
#ifdef DEBUG
      std::cout << "Optimal Solution FOUND.\n"
                << "Solver took: " << timing.realTime << "ms (Real Time)"
                << "Solver took: " << timing.cpuTime << "ms (CPU Time)"
                << std::endl;
#endif
    }
  else
    {
      m_optimalSolutionFound = false;
#ifdef DEBUG
      std::cout << "Optimal Solution NOT FOUND.\n"
                << "Solver took: " << timing.realTime << "ms (Real Time)"
                << "Solver took: " << timing.cpuTime << "ms (CPU Time)"
                << std::endl;
#endif
    }
}

void
GraphManager::LogDuration (tinyxml2::XMLDocument& xmlDoc)
{
  using namespace tinyxml2;
  XMLNode* rootNode = XmlUtilities::GetRootNode (xmlDoc);

  XMLElement* durationElement = xmlDoc.NewElement ("Duration");
  durationElement->SetAttribute ("total_real_duration_ms",
                                 m_maxFlowTiming.realTime + m_minCostTiming.realTime);
  durationElement->SetAttribute ("total_cpu_duration_ms",
                                 m_maxFlowTiming.cpuTime + m_minCostTiming.cpuTime);

  XMLElement* maxFlowElement = xmlDoc.NewElement ("MaximumFlow");
  maxFlowElement->SetAttribute ("real_duration_ms", m_maxFlowTiming.realTime);
  maxFlowElement->SetAttribute ("cpu_duration_ms", m_maxFlowTiming.cpuTime);
  durationElement->InsertEndChild (maxFlowElement);

  XMLElement* minCostElement = xmlDoc.NewElement ("MinimumCost");
  minCostElement->SetAttribute ("real_duration_ms", m_minCostTiming.realTime);
  minCostElement->SetAttribute ("cpu_duration_ms", m_minCostTiming.cpuTime);
  durationElement->InsertEndChild (minCostElement);

  rootNode->InsertFirstChild (durationElement);
}

void
GraphManager::LogOptimalSolution (tinyxml2::XMLDocument& xmlDoc)
{
  using namespace tinyxml2;
  XMLNode* rootNode = XmlUtilities::GetRootNode (xmlDoc);

  XMLElement* optimalSolutionElement = xmlDoc.NewElement ("OptimalSolution");

  // Looping through all the flows in reverse order.
  for (auto flow = m_flows->rbegin(); flow != m_flows->rend(); ++flow)
    {
      XMLElement* flowElement = xmlDoc.NewElement ("Flow");
      flowElement->SetAttribute ("Id", (*flow).id);
      flowElement->SetAttribute ("SourceNode", (*flow).source);
      flowElement->SetAttribute ("DestinationNode", (*flow).destination);

      if ((*flow).protocol == FlowManager::Flow::Protocol::Tcp)
        {
          flowElement->SetAttribute ("SrcPortNumber", (*flow).srcPortNumber);
          flowElement->SetAttribute ("DstPortNumber", (*flow).dstPortNumber);
        }
      else
        {
          flowElement->SetAttribute ("PortNumber", (*flow).dstPortNumber);
        }

      flowElement->SetAttribute ("DataRate", (*flow).dataRate);
      flowElement->SetAttribute ("PacketSize", (*flow).packetSize);
      flowElement->SetAttribute ("NumOfPackets", (*flow).numOfPackets);
      flowElement->SetAttribute ("Protocol", std::string (1, (*flow).protocol).c_str());
      flowElement->SetAttribute ("StartTime", (*flow).startTime);
      flowElement->SetAttribute ("EndTime", (*flow).endTime);

      // Looping through the optimal solution
      for (lemon::SmartDigraph::ArcIt link (m_graph); link != lemon::INVALID; ++link)
        {
          double flowRatio =
            m_lpSolver.primal (m_optimalFlowRatio[std::make_pair ((*flow).id, link)]);

          if (flowRatio > 0)
            {
              // Add link element
              XMLElement* linkElement = xmlDoc.NewElement ("Link");
              linkElement->SetAttribute ("Id", m_graph.id (link));
              linkElement->SetAttribute ("FlowRate", flowRatio);
              flowElement->InsertFirstChild (linkElement);
            }
        }
      optimalSolutionElement->InsertFirstChild (flowElement);
    }

  XMLComment* unitsComment = xmlDoc.NewComment ("DataRate (Mbps), PacketSize (bytes),"
                             "Protocol (U=UDP,T=TCP), Time (Seconds)."
                             "\nUnless Specified the port number refers "
                             "to the destination port number");

  optimalSolutionElement->InsertFirstChild (unitsComment);
  rootNode->InsertEndChild (optimalSolutionElement);
}

void
GraphManager::LogIncomingFlow (tinyxml2::XMLDocument& xmlDoc)
{
  struct FlowPair
  {
    uint32_t Id;
    double flowValue;
  };

  // Node Id = key, FlowPair = value
  std::map<uint32_t, std::vector<FlowPair>> incomingFlow;
  for (const FlowManager::Flow& flow : *m_flows) // Looping through all the flows.
    {
      for (lemon::SmartDigraph::ArcIt link (m_graph); link != lemon::INVALID; ++link)
        {
          uint32_t destinationId = m_graph.id (m_graph.target (link));
          double flowValue = m_lpSolver.primal (m_optimalFlowRatio[std::make_pair (flow.id, link)]);

          bool flowFound (false);
          for (auto& flowPair : incomingFlow[destinationId])
            {
              if (flow.id == flowPair.Id)
                {
                  flowPair.flowValue += flowValue;
                  flowFound = true;
                }
            }
          if (!flowFound)
            {
              FlowPair currentPair;
              currentPair.Id = flow.id;
              currentPair.flowValue = flowValue;
              incomingFlow[destinationId].push_back (currentPair);
            }
        }
    }

  // Sorting the vector
  std::sort (incomingFlow[0].begin(), incomingFlow[0].end(), [] (const FlowPair & lhs,
             const FlowPair & rhs)
  {
    return lhs.Id < rhs.Id;
  });

  // Exporting the incoming flow to the XML log file.
  using namespace tinyxml2;
  XMLNode* rootNode = XmlUtilities::GetRootNode (xmlDoc);

  XMLElement* incomingFlowElement = xmlDoc.NewElement ("IncomingFlow");

  for (const auto& incomingElement : incomingFlow)
    {
      // Create an element for that node.
      XMLElement* nodeElement = xmlDoc.NewElement ("Node");
      nodeElement->SetAttribute ("Id", incomingElement.first);

      for (const auto& flowDetails : incomingElement.second)
        {
          if (flowDetails.flowValue > 0) // Do not store elements that have a flow value of 0
            {
              XMLElement* flowElement = xmlDoc.NewElement ("Flow");
              flowElement->SetAttribute ("Id", flowDetails.Id);
              flowElement->SetAttribute ("IncomingFlow", flowDetails.flowValue);
              nodeElement->InsertEndChild (flowElement);
            }
        }
      // If this element has no children do not store it in the XML file
      if (!nodeElement->NoChildren())
        incomingFlowElement->InsertEndChild (nodeElement);
    }
  rootNode->InsertEndChild (incomingFlowElement);
}

void
GraphManager::LogNetworkTopology (tinyxml2::XMLDocument& xmlDoc)
{
  using namespace tinyxml2;
  XMLNode* rootNode = XmlUtilities::GetRootNode (xmlDoc);

  XMLElement* networkTopologyElement = xmlDoc.NewElement ("NetworkTopology");
  networkTopologyElement->SetAttribute ("NumberOfNodes", lemon::countNodes (m_graph));
  networkTopologyElement->SetAttribute ("NumberOfLinks", lemon::countArcs (m_graph));

  std::set<int> visitedLinks;

  // We need to loop through all the links and add their details.
  for (lemon::SmartDigraph::ArcIt link (m_graph); link != lemon::INVALID; ++link)
    {
      int linkId (m_graph.id (link));
      int oppositeLinkId (0);

      // This link is already stored in the XML file. Skip it.
      if (visitedLinks.find (linkId) != visitedLinks.end()) continue;

      lemon::SmartDigraph::Arc oppositeLink;

      double currentLinkDelay (m_linkDelay[link]);
      bool pairFound (false);

      // Check if link with opposite source and destination exists.
      for (lemon::ConArcIt<lemon::SmartDigraph> oppositeLinkIt (m_graph,
           m_graph.target (link),
           m_graph.source (link));
           oppositeLinkIt != lemon::INVALID; ++oppositeLinkIt)
        {
          oppositeLinkId = m_graph.id (oppositeLinkIt);
          // This link is already stored in the XML file. Skip it.
          if (visitedLinks.find (oppositeLinkId) != visitedLinks.end()) continue;

          if (currentLinkDelay == m_linkDelay[oppositeLinkIt])
            {
              pairFound = true;
              oppositeLink = oppositeLinkIt;
              break;
            }
        }

      if (!pairFound) // If no pair was found issue a warning.
        {
          std::cerr << "Warning: Link " << linkId << " has no opposite link." << std::endl;
          visitedLinks.insert (linkId);

          XMLElement* linkElement = xmlDoc.NewElement ("Link");
          linkElement->SetAttribute ("Delay", m_linkDelay[link]);
          linkElement->InsertFirstChild (CreateLinkElement (xmlDoc, link));

          networkTopologyElement->InsertFirstChild (linkElement);
        }
      else // A pair is found.
        {
          assert (m_linkDelay[link] == m_linkDelay[oppositeLink]);
          visitedLinks.insert (linkId);
          visitedLinks.insert (oppositeLinkId);

          XMLElement* linkElement = xmlDoc.NewElement ("Link");
          linkElement->SetAttribute ("Delay", m_linkDelay[link]);
          linkElement->InsertFirstChild (CreateLinkElement (xmlDoc, link));
          linkElement->InsertFirstChild (CreateLinkElement (xmlDoc, oppositeLink));

          networkTopologyElement->InsertFirstChild (linkElement);
        }
    }

  // Add a comment in the XML file that will describe the units being used.
  XMLComment* unitsComment =
    xmlDoc.NewComment ("Delay (ms), Capacity (Mbps), Node Type (T=Terminal, S=Switch)");
  networkTopologyElement->InsertFirstChild (unitsComment);
  rootNode->InsertEndChild (networkTopologyElement);
}

void
GraphManager::LogNodeConfiguration (tinyxml2::XMLDocument& xmlDoc)
{
  // Here we need to log the node's configuration parameters!
  using namespace tinyxml2;
  XMLNode* rootNode = XmlUtilities::GetRootNode (xmlDoc);

  XMLElement* nodeConfiguration = xmlDoc.NewElement ("NodeConfiguration");

  for (lemon::SmartDigraph::NodeIt node (m_graph); node != lemon::INVALID; ++node)
    {
      XMLElement* nodeElement = xmlDoc.NewElement ("Node");
      nodeElement->SetAttribute ("Id", m_graph.id (node));
      nodeElement->SetAttribute ("Type", std::string (1, m_nodeType[node]).c_str());
      nodeElement->SetAttribute ("X", m_nodeCoordinates[node].x);
      nodeElement->SetAttribute ("Y", m_nodeCoordinates[node].y);

      nodeConfiguration->InsertFirstChild (nodeElement);
    }

  rootNode->InsertEndChild (nodeConfiguration);
}

void
GraphManager::LogFlowDataRateUpdates (tinyxml2::XMLDocument &xmlDoc)
{
  if ( m_modifiedFlows.size() > 0 )
    {
      // Logging the flows that their data rates were modified
      using namespace tinyxml2;
      XMLNode* rootNode = XmlUtilities::GetRootNode (xmlDoc);

      XMLElement* flowDataRateModElement = xmlDoc.NewElement ("FlowDataRateModifications");
      for (auto& modFlow : m_modifiedFlows)
        {
          XMLElement* flowElement = xmlDoc.NewElement ("Flow");
          flowElement->SetAttribute ("Id", modFlow.id);
          flowElement->SetAttribute ("RequestedDataRate", modFlow.requestedDataRate);
          flowElement->SetAttribute ("ReceivedDataRate", modFlow.receivedDataRate);
          flowDataRateModElement->InsertEndChild (flowElement);
        }

      rootNode->InsertEndChild (flowDataRateModElement);
    }
}

tinyxml2::XMLElement*
GraphManager::CreateLinkElement (tinyxml2::XMLDocument& xmlDoc, lemon::SmartDigraph::Arc& link)
{
  using namespace tinyxml2;
  lemon::SmartDigraph::Node sourceNode = m_graph.source (link);
  lemon::SmartDigraph::Node destinationNode = m_graph.target (link);
  int sourceNodeId = m_graph.id (sourceNode);
  int destinationNodeId = m_graph.id (destinationNode);

  XMLElement* linkElement = xmlDoc.NewElement ("LinkElement");

  linkElement->SetAttribute ("Id", m_graph.id (link));
  linkElement->SetAttribute ("SourceNode", sourceNodeId);
  linkElement->SetAttribute ("SourceNodeType",
                             std::string (1, m_nodeType[sourceNode]).c_str());
  linkElement->SetAttribute ("DestinationNode", destinationNodeId);
  linkElement->SetAttribute ("DestinationNodeType",
                             std::string (1, m_nodeType[destinationNode]).c_str());
  linkElement->SetAttribute ("Capacity", m_linkCapacity[link]);

  return linkElement;
}
