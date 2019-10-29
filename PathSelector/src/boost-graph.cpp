#include <set>
#include <list>
#include <random>
#include <math.h>
#include <boost/graph/copy.hpp>
#include <boost/numeric/conversion/cast.hpp>

#include "boost-graph.hpp"
#include "yen_ksp.hpp"

/**
 A function that compares two floating point numbers to check for equality.

 @param value1 The first value to compare.
 @param value2 The second value to compare.
 @param accuracy The accuracy used during the comparison. Default: 1e-9.
 @return True: The numbers are equal at the given accuracy.
         False: The numbers are not equal at the given accuracy.
 */
bool
numbersAreClose (double value1, double value2, double accuracy = 1e-9)
{
  return (std::fabs (value1 - value2) < accuracy);
}

/**
 Construct the BoostGraph object from the lemon graph.

 @param lemonGraph Instance of the LemonGraph object.
 */
BoostGraph::BoostGraph (const LemonGraph &lemonGraph)
{
  GenerateBoostGraph (lemonGraph);
}

/**
 Generate the boost graph equivalent of the given Lemon graph.

 @param lemonGraph Instance of the LemonGraph object.
 */
void
BoostGraph::GenerateBoostGraph (const LemonGraph &lemonGraph)
{
  GenerateBoostNodes (lemonGraph);
  GenerateBoostLinks (lemonGraph);
  LOG_MSG ("LEMON Graph converted to BOOST Graph successfully");
}

/**
 Generates the equivalent boost graph nodes from the Lemon graph.

 @param lemonGraph Instance of the LemonGraph object.
 */
void
BoostGraph::GenerateBoostNodes (const LemonGraph &lemonGraph)
{
  LOG_MSG ("Building nodes...");
  for (auto lemonNode = lemonGraph.GetNodeIt (); lemonNode != lemon::INVALID; ++lemonNode)
    {
      auto nodeId{lemonGraph.GetNodeId (lemonNode)};
      auto nodeType{lemonGraph.GetNodeType (lemonNode)};

      auto boostNode{boost::add_vertex ({nodeId, nodeType}, m_graph)};
      auto ret = m_nodeMap.emplace (nodeId, boostNode);
      if (!ret.second)
        {
          throw std::runtime_error (
              "Trying to insert a duplicate node. Node Id: " + std::to_string (nodeId) + "\n");
        }

      LOG_MSG ("Added node " << boost::get (&NodeDetails::id, m_graph, boostNode) << " Type "
                             << boost::get (&NodeDetails::type, m_graph, boostNode));
    }
}

/**
 Generates the equivalent boost graph links from the Lemon graph.

 @param lemonGraph Instance of the LemonGraph object.
 */
void
BoostGraph::GenerateBoostLinks (const LemonGraph &lemonGraph)
{
  LOG_MSG ("Building links...");
  for (auto lemonLink = lemonGraph.GetLinkIt (); lemonLink != lemon::INVALID; ++lemonLink)
    {
      auto srcNodeId = lemonGraph.GetNodeId (lemonGraph.GetSourceNode (lemonLink));
      auto dstNodeId = lemonGraph.GetNodeId (lemonGraph.GetDestinationNode (lemonLink));

      auto linkId = lemonGraph.GetLinkId (lemonLink);
      auto linkCost = lemonGraph.GetLinkCost (lemonLink);
      auto linkCapacity = lemonGraph.GetLinkCapacity (lemonLink);

      link_t boostLink;
      bool linkAdded{false};
      std::tie (boostLink, linkAdded) =
          boost::add_edge (m_nodeMap.at (srcNodeId), m_nodeMap.at (dstNodeId),
                           {linkId, linkCost, linkCapacity}, m_graph);

      if (!linkAdded)
        {
          throw std::runtime_error (
              "Link could not be added in Boost graph. Link Id: " + std::to_string (linkId) + "\n");
        }

      auto ret = m_linkMap.emplace (linkId, boostLink);
      if (!ret.second)
        {
          throw std::runtime_error ("Failed to insert link " + std::to_string (linkId) +
                                    " in the link map");
        }

#ifdef MY_DEBUG /* Debug only Logging */
      auto srcNode{boost::source (boostLink, m_graph)};
      auto dstNode{boost::target (boostLink, m_graph)};
#endif
      LOG_MSG ("Added link " << boost::get (&LinkDetails::id, m_graph, boostLink) << " Cost "
                             << boost::get (&LinkDetails::cost, m_graph, boostLink) << " Capacity "
                             << boost::get (&LinkDetails::capacity, m_graph, boostLink)
                             << " Source Node " << boost::get (&NodeDetails::id, m_graph, srcNode)
                             << " Destination Node "
                             << boost::get (&NodeDetails::id, m_graph, dstNode));
    }
}

/**
 Return the boost link given a link id.

 @param linkId The link id.
 @return The boost graph link.
 */
BoostGraph::link_t
BoostGraph::GetLink (id_t linkId) const
{
  try
    {
      return m_linkMap.at (linkId);
  } catch (const std::out_of_range &oor)
    {
      std::cerr << "The link " << linkId << " was not found" << std::endl;
      throw;
  }
}

/**
 Returns the link id given a boost graph link.

 @param link The boost graph link.
 @return The link id.
 */
id_t
BoostGraph::GetLinkId (const BoostGraph::link_t &link) const
{
  return boost::get (&LinkDetails::id, m_graph, link);
}

/**
 Returns the cost of the given boost graph link.

 @param link The boost graph link.
 @return The link cost.
 */
linkCost_t
BoostGraph::GetLinkCost (const BoostGraph::link_t &link) const
{
  return boost::get (&LinkDetails::cost, m_graph, link);
}

/**
 Returns the capacity of the given boost graph link.

 @param link The boost graph link.
 @return The link capacity.
 */
linkCapacity_t
BoostGraph::GetLinkCapacity (const BoostGraph::link_t &link) const
{
  return boost::get (&LinkDetails::capacity, m_graph, link);
}

/**
 @brief Retrieve the link opposite to that given by \p linkId.

 Retrieve the link opposite to that given by \p linkId. The opposite link is
 defined as the link that has the opposite source and destination nodes BUT
 identical delay values. The capacities may be different.

 If the opposite link has not been found, the returned link id is equal to \p
 linkId.

 @param linkId The id of the link to find the opposite of.
 @return The link id of the opposite link. If the opposite link is not found,
 the returned link id is equal to that given.
 */
std::list<id_t>
BoostGraph::GetOppositeLink (id_t linkId) const
{
  auto link = GetLink (linkId);
  auto linkCost{GetLinkCost (link)};

  auto srcNode{boost::source (link, m_graph)};
  auto dstNode{boost::target (link, m_graph)};
  auto dstNodeId{GetNodeId (dstNode)};

  std::list<id_t> oppositeLinks;
  auto incomingLinksIterators{boost::in_edges (srcNode, m_graph)};

  for (auto incomingLinkIt = incomingLinksIterators.first;
       incomingLinkIt != incomingLinksIterators.second; ++incomingLinkIt)
    {
      auto incomingLinkSrcNodeId = GetNodeId (boost::source (*incomingLinkIt, m_graph));
      auto incomingLinkCost = GetLinkCost (*incomingLinkIt);

      // FIXME: Update the below comparison
      if ((incomingLinkSrcNodeId == dstNodeId) && (linkCost == incomingLinkCost))
        {
          oppositeLinks.emplace_back (GetLinkId (*incomingLinkIt));
        }
    }

  if (oppositeLinks.empty ())
    {
      std::cout << "Warning: Link " << linkId << " has no opposite link" << std::endl;
    }

  return oppositeLinks;
}

/**
 Get iterators over the boost graph links.

 @return Iterator over the boost graph links.
 */
std::pair<BoostGraph::graph_t::edge_iterator, BoostGraph::graph_t::edge_iterator>
BoostGraph::GetLinkIterators () const
{
  return boost::edges (m_graph);
}

/**
 Returns the node id given a boost graph node.

 @param node The boost graph node.
 @return The node id.
 */
id_t
BoostGraph::GetNodeId (const BoostGraph::node_t &node) const
{
  return boost::get (&NodeDetails::id, m_graph, node);
}

/**
 @brief Returns the node type for the given boost graph node.

 Returns the node type for the given boost graph node. A node type of
 'S' means the node is a switch. A node type of 'T' means the node is
 a terminal.

 @param node The boost graph node.
 @return The node type.
 */
char
BoostGraph::GetNodeType (node_t node) const
{
  return boost::get (&NodeDetails::type, m_graph, node);
}

/**
 Returns the source node of the given link.

 @param link The boost graph link.
 @return The source node of the given link.
 */
BoostGraph::node_t
BoostGraph::GetSourceNode (const link_t &link) const
{
  return boost::source (link, m_graph);
}

/**
 Returns the destination node of the given link.

 @param link The boost graph link.
 @return The destination node of the given link.
 */
BoostGraph::node_t
BoostGraph::GetDestinationNode (const link_t &link) const
{
  return boost::target (link, m_graph);
}

// TODO: Add documentation
void
BoostGraph::GetPaths (Flow::flowContainer_t &flows)
{
  for (auto &[flowId, flow] : flows)
    {
      LOG_MSG ("Finding " << flow.k << " paths for flow: " << flowId);

      auto k = flow.k;
      auto &srcNode{m_nodeMap.at (flow.sourceId)};
      auto &dstNode{m_nodeMap.at (flow.destinationId)};

      auto numPathsToGet = k + 1;
      pathContainer_t paths;
      auto lastPathCost = linkCost_t{0.0};
      auto secondToLastPathCost = linkCost_t{0.0};
      auto randomlyRemoveExcessPaths{true};

      do
        {
          // FIXME: Replace with the function
          paths = boost::yen_ksp (m_graph, srcNode, dstNode,
                                  /* Link weight attribute */
                                  boost::get (&LinkDetails::cost, m_graph),
                                  boost::get (boost::vertex_index_t (), m_graph), numPathsToGet);

          lastPathCost = paths.back ().first;
          secondToLastPathCost = std::prev (paths.end (), 2)->first;

          if (paths.size () <= k)
            {
              randomlyRemoveExcessPaths = false;
              break;
            }
          else if (paths.empty ())
            throw std::runtime_error ("No paths were found for flow " + std::to_string (flow.id));

          numPathsToGet++;
        }
      while (numbersAreClose (lastPathCost, secondToLastPathCost));

      if (randomlyRemoveExcessPaths == false)
        {
          AddDataPaths (flow, paths);
        }
      else
        { // FIXME: Put this in a function
          // Container storing the list of paths to be chosen at random
          pathContainer_t pathsWithEqualCost;

          auto pathIterator = paths.begin ();
          std::advance (pathIterator, k - 1);
          auto kthPathCost = pathIterator->first;

          for (; pathIterator != paths.end (); ++pathIterator)
            {
              if (numbersAreClose (pathIterator->first, kthPathCost))
                pathsWithEqualCost.push_back (*pathIterator);
            }

          pathContainer_t randomlyChosenPaths;
          std::sample (pathsWithEqualCost.begin (), pathsWithEqualCost.end (),
                       std::back_inserter (randomlyChosenPaths), 1,
                       std::mt19937{std::random_device{}()});

          pathContainer_t finalPathSet;
          finalPathSet.assign (paths.begin (), std::next (paths.begin (), k - 1));
          finalPathSet.assign (randomlyChosenPaths.begin (), randomlyChosenPaths.end ());

          AddDataPaths (flow, finalPathSet);
        }
    }
}

/**
 * @brief Return the K shortest paths between the given source and destination nodes
 * @param srcNode The Source Node
 * @param dstNode The Destination Node
 * @param k The number of paths to find
 * @return The list of paths found
 */
BoostGraph::pathContainer_t
BoostGraph::GetKShortestPaths (node_t srcNode, node_t dstNode, uint32_t k)
{
  return boost::yen_ksp (m_graph, srcNode, dstNode,
                         /* Link weight attribute */
                         boost::get (&LinkDetails::cost, m_graph),
                         boost::get (boost::vertex_index_t (), m_graph), k);
}

/**
 * @brief Return the K shortest edge disjoint paths between the given source and destination nodes
 * @param srcNode The Source Node
 * @param dstNode The Destination Node
 * @param k The number of paths to find
 * @return The list of paths found
 */
BoostGraph::pathContainer_t
BoostGraph::GetKShortestEdgeDisjointPaths (node_t srcNode, node_t dstNode, uint32_t k)
{
  graph_t tempGraph;
  boost::copy_graph (m_graph, tempGraph);

  auto pathFound = bool{false};
  auto numPathsFound = uint32_t{0};

  pathContainer_t paths;

  do
    {
      auto path =
          pathContainer_t{boost::yen_ksp (m_graph, srcNode, dstNode,
                                          /* Link weight attribute */
                                          boost::get (&LinkDetails::cost, m_graph),
                                          boost::get (boost::vertex_index_t (), m_graph), 1)};

      if (path.empty ())
        pathFound = false;

      // Remove all edges of the found path
      for (const auto &[linkCost, links] : path)
        {
          for (const auto &link : links)
            {
              auto srcNode = boost::source (link, tempGraph);
              auto dstNode = boost::target (link, tempGraph);

              auto srcNodeType = boost::get (&NodeDetails::type, tempGraph, srcNode);
              auto dstNodeType = boost::get (&NodeDetails::type, tempGraph, dstNode);

              if (srcNodeType == 'S' && dstNodeType == 'S')
                { // Only remove edges between switches
                  LOG_MSG ("Removing link " << boost::get (&LinkDetails::id, tempGraph, link)
                                            << " Cost" << linkCost);
                  boost::remove_edge (link, tempGraph);
                }
            }
        }

      // Move the found path to the paths list
      paths.splice (paths.end (), path);

      numPathsFound++;
    }
  while (pathFound || (numPathsFound < k)); // FIXME: Verify that this is correct

  return paths;
}

///**
// * @brief Find the first K relaxed edge disjoint paths for each flow.
// *
// * The Relaxed Edge disjoint algorithm will find the first K edge disjoint
// * paths; however, different from the algorithm in FindKEdgeDisjointPaths, links
// * that are the only means of communication for that node are not deleted.
// *
// * The links that will not be deleted are discovered using the following method
// * after having found the shortest path that links the source with the
// * destination node. First, the shortest path is traversed from the source node,
// * to the destination nodes. For every node met, a check is made to see if it's
// * the only outgoing link available to that node. If it is, retain that link, if
// * not the link can be deleted in subsequent iterations. The same procedure is
// * repeated starting from the destination node and moving towards the source
// * node backwards. The link is retained if nodes only have one incoming link.
// *
// * @param[in,out] flows The flow set that will be updated with the found paths.
// */
//void
//BoostGraph::FindKRelaxedEdgeDisjointPaths (Flow::flowContainer_t &flows)
//{
//  for (auto &flowPair : flows)
//    {
//      auto &flow{flowPair.second};

//      LOG_MSG ("Finding " << flow.k << " relaxed edge disjoint path(s) for Flow: " << flow.id);

//      // Find and save links that cannot be removed as they are the sole point of
//      // connection
//      auto linksToRetain = std::set<id_t>{GetLinksToRetain (flow)};

//      graph_t tempGraph;
//      boost::copy_graph (m_graph, tempGraph);

//      // Container storing the edge disjoint paths as a list of link ids
//      std::list<std::list<id_t>> edgeDisjointPaths;

//      for (uint32_t i = 0; i < flow.k; ++i)
//        {
//          auto &srcNode{m_nodeMap.at (flow.sourceId)};
//          auto &dstNode{m_nodeMap.at (flow.destinationId)};

//          auto shortestPath =
//              pathContainer_t{boost::yen_ksp (tempGraph, srcNode, dstNode,
//                                              /* Link weight attribute */
//                                              boost::get (&LinkDetails::cost, tempGraph),
//                                              boost::get (boost::vertex_index_t (), tempGraph), 1)};

//          if (shortestPath.empty ())
//            break; // No more paths found. Exit the loop

//          // Save the found path
//          LOG_MSG_NONEWLINE ("Path " << i << " Found. Links: ");
//          std::list<id_t> pathLinks;
//          for (const auto &shortestPathDetails : shortestPath)
//            {
//              for (const auto &link : shortestPathDetails.second)
//                {
//                  auto linkId{boost::get (&LinkDetails::id, tempGraph, link)};
//                  pathLinks.emplace_back (linkId);
//                  LOG_MSG_NONEWLINE (linkId << ", ");
//                }
//            }
//          LOG_MSG_NONEWLINE ("\n");

//          if (edgeDisjointPaths.back () == pathLinks)
//            {
//              LOG_MSG ("This path has been found already. All paths for Flow: "
//                       << flow.id << " have been found");
//              break;
//            }

//          edgeDisjointPaths.push_back (pathLinks);

//          /**
//       * Remove all the edges of the found path with the exception of the links
//       * that are in the links to retain set.
//       */
//          for (const auto &shortestPathDetails : shortestPath)
//            {
//              for (const auto &link : shortestPathDetails.second)
//                {
//                  auto linkId = id_t{boost::get (&LinkDetails::id, tempGraph, link)};

//                  if (linksToRetain.find (linkId) != linksToRetain.end ())
//                    { // The link should not be deleted
//                      LOG_MSG ("Link: " << linkId << " has been retained");
//                    }
//                  else
//                    {
//                      boost::remove_edge (link, tempGraph);
//                      LOG_MSG ("Link: " << linkId << " has been removed");
//                    }
//                }
//            }
//        }

//      if (edgeDisjointPaths.empty ())
//        throw std::runtime_error ("No paths were found for flow " + std::to_string (flow.id));

//      // Save the paths to the flow
//      for (const auto &path : edgeDisjointPaths)
//        {
//          Path dataPath (/* assign a path id to this path */ true);
//          auto pathCost = 0.0;

//          for (const auto &linkId : path)
//            {
//              pathCost += boost::get (&LinkDetails::cost, m_graph, GetLink (linkId));
//              dataPath.AddLink (linkId);
//            }

//          dataPath.cost = pathCost;
//          flow.AddDataPath (dataPath);
//        }
//    }
//}

std::set<id_t>
BoostGraph::GetLinksToRetain (const Flow &flow)
{
  auto &flowSrcNode{m_nodeMap.at (flow.sourceId)};
  auto &flowDstNode{m_nodeMap.at (flow.destinationId)};

  auto foundPaths = pathContainer_t{boost::yen_ksp (
      m_graph, flowSrcNode, flowDstNode,
      /* Link weight attribute */
      boost::get (&LinkDetails::cost, m_graph), boost::get (boost::vertex_index_t (), m_graph), 1)};

  if (foundPaths.empty ())
    {
      throw std::runtime_error ("There is no path for flow " + std::to_string (flow.id));
    }

  // Retrieve the shortest path
  const auto &shortestPath = foundPaths.front ().second;

  // Store the link ids that must not be removed
  std::set<id_t> linksToRetain;

  // Forward Search
  LOG_MSG ("Starting the forward search for Flow: " << flow.id << "...");
  for (const auto &link : shortestPath)
    {
      auto srcNode{GetSourceNode (link)};
      // The number of outgoing links from a node excluding those connected to a
      // terminal
      auto numOutgoingLinks = uint32_t{0};

      using outEdgeIt = graph_t::out_edge_iterator;
      outEdgeIt ei, eiEnd;
      for (boost::tie (ei, eiEnd) = boost::out_edges (srcNode, m_graph); ei != eiEnd; ++ei)
        {
          if (GetNodeType (GetDestinationNode (*ei)) == 'T')
            continue;
          else
            numOutgoingLinks++;
        }
      LOG_MSG ("  Node: " << GetNodeId (srcNode) << " has " << numOutgoingLinks
                          << " outgoing link(s)");

      if (numOutgoingLinks <= 1)
        {
          auto linkId{GetLinkId (link)};
          LOG_MSG ("  Link: " << linkId << " retained");
          linksToRetain.emplace (linkId);
        }
      else
        break;
    }
  LOG_MSG ("Forward search for Flow: " << flow.id << " complete");

  // Backward Search
  LOG_MSG ("Starting the backward search for Flow: " << flow.id << "...");
  using revPathIt_t = std::list<BoostGraph::link_t>::const_reverse_iterator;
  for (revPathIt_t linkIt = shortestPath.rbegin (); linkIt != shortestPath.rend (); ++linkIt)
    {
      const auto &link = *linkIt;
      auto dstNode{GetDestinationNode (link)};
      // The number of incoming links from a node excluding those connected with a
      // terminal
      auto numIncomingLinks = uint32_t{0};

      using inEdgeIt = graph_t::in_edge_iterator;
      inEdgeIt ei, eiEnd;
      for (boost::tie (ei, eiEnd) = boost::in_edges (dstNode, m_graph); ei != eiEnd; ++ei)
        {
          if (GetNodeType (GetSourceNode (*ei)) == 'T')
            continue;
          else
            numIncomingLinks++;
        }
      LOG_MSG ("  Node: " << GetNodeId (dstNode) << " has " << numIncomingLinks
                          << " incoming link(s)");

      if (numIncomingLinks <= 1)
        {
          auto linkId{GetLinkId (link)};
          LOG_MSG ("  Link: " << linkId << " retained");
          linksToRetain.emplace (linkId);
        }
      else
        break;
    }
  LOG_MSG ("Backward search for Flow: " << flow.id << " complete");

  return linksToRetain;
}

/**
 Update the flow object to include the paths returned by the KSP algorithm.

 @param flow The flow object to update.
 @param paths The paths to add to the flow.
 */
void
BoostGraph::AddDataPaths (Flow &flow, const BoostGraph::pathContainer_t &paths)
{
  for (const auto &path : paths)
    {
      Path dataPath (/* assign a path id to this path */ true);
      dataPath.cost = path.first;

      for (const auto &link : path.second)
        {
          dataPath.AddLink (boost::get (&LinkDetails::id, m_graph, link));
        }
      flow.AddDataPath (dataPath);
    }
}

/**
 @brief Find the routes that the Acknowledgment flows will take for TCP flows.

 Find the routes that the Acknowledgment flows will take for TCP flows by
 looping through all the paths of each flow and finding the reverse path for
 each data path in the flow.

 @param[in,out] flows The flow container.
 */
void
BoostGraph::AddAckPaths (Flow::flowContainer_t &flows)
{
  for (auto &flowPair : flows)
    {
      auto &flow{flowPair.second};

      if (flow.protocol == Protocol::Udp)
        { // No ack paths necessary for UDP flows
          continue;
        }

      LOG_MSG ("Add ACK paths for Flow: " << flow.id);

      for (const auto &path : flow.GetDataPaths ())
        {
          Path ackPath (/* do not assign a path id to this path */ false);
          ackPath.id = path.id; // Set the Ack Path id to be identical to the Data path id

          for (const auto &link : path.GetLinks ())
            {
              LOG_MSG ("We are working on link " << link);
              auto &dataLink = m_linkMap.at (link);

              auto dataSrcNode = boost::source (dataLink, m_graph);
              auto dataDstNode = boost::target (dataLink, m_graph);

              bool ackLinkFound{false};
              BoostGraph::link_t ackLink;

              // The source and destination nodes are reversed to find the opposite
              // link
              std::tie (ackLink, ackLinkFound) = boost::edge (dataDstNode, dataSrcNode, m_graph);

              if (!ackLinkFound)
                {
                  throw std::runtime_error ("The opposite link for link " + std::to_string (link) +
                                            " has not been found");
                }
              ackPath.AddLink (boost::get (&LinkDetails::id, m_graph, ackLink));
            }

          flow.AddAckPath (ackPath);
        }
    }
}

/**
 @brief Find the shortest route that the Acknowledgement flow can take.

 Find the shortest route that the Acknowledgement flow can take. This ack route
 will be used by the network simulator when the PPFS switch will be used where
 only a single path for the ACK is needed.

 @param[in,out] flows The flow container.
 */
void
BoostGraph::AddShortestPathAck (Flow::flowContainer_t &flows)
{
  for (auto &flowPair : flows)
    {
      auto &flow{flowPair.second};

      auto &srcNode{m_nodeMap.at (flow.sourceId)};
      auto &dstNode{m_nodeMap.at (flow.destinationId)};

      auto ackPathContainer =
          pathContainer_t{boost::yen_ksp (m_graph, dstNode, srcNode,
                                          /* Link weight attribute */
                                          boost::get (&LinkDetails::cost, m_graph),
                                          boost::get (boost::vertex_index_t (), m_graph), 1)};

      if (ackPathContainer.empty ())
        {
          throw std::runtime_error ("No paths were found for flow " + std::to_string (flow.id));
        }
      else
        {
          const auto &ackPathPair{ackPathContainer.front ()};
          Path ackPath (false);
          ackPath.cost = ackPathPair.first;

          for (const auto &link : ackPathPair.second)
            {
              ackPath.AddLink (boost::get (&LinkDetails::id, m_graph, link));
            }

          flow.AddAckShortestPath (ackPath);
        }
    }
}
