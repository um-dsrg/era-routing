#include <set>
#include <list>
#include <random>
#include <math.h>
#include <functional>
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

bool
BoostGraph::LinkExists (id_t linkId) const
{
  return (m_linkMap.find (linkId) != m_linkMap.end ());
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
      std::cerr << "The link " << linkId << " was not found\n" << oor.what () << std::endl;
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

      if ((incomingLinkSrcNodeId == dstNodeId) && (numbersAreClose (linkCost, incomingLinkCost)))
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
BoostGraph::AssignPathsToFlows (Flow::flowContainer_t &flows,
                                const std::string &pathSelectionAlgorithm)
{
  std::function<pathContainer_t (node_t, node_t, uint32_t)> pathSelectorFunction;

  if (pathSelectionAlgorithm == "KSP")
    pathSelectorFunction = std::bind (&BoostGraph::GetKShortestPaths, this, std::placeholders::_1,
                                      std::placeholders::_2, std::placeholders::_3);
  else if (pathSelectionAlgorithm == "RED")
    pathSelectorFunction =
        std::bind (&BoostGraph::GetKShortestRelaxedEdgeDisjointPaths, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3);
  else if (pathSelectionAlgorithm == "ED")
    pathSelectorFunction =
        std::bind (&BoostGraph::GetKShortestEdgeDisjointPaths, this, std::placeholders::_1,
                   std::placeholders::_2, std::placeholders::_3);
  else
    throw std::runtime_error ("The path selection algorithm " + pathSelectionAlgorithm +
                              "is not supported");

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

      auto previousNumPathsFound = size_t{0};

      do
        {
          previousNumPathsFound = paths.size ();
          paths = pathSelectorFunction (srcNode, dstNode, numPathsToGet);

          lastPathCost = paths.back ().first;
          secondToLastPathCost = std::prev (paths.end (), 2)->first;

          if (paths.empty ())
            throw std::runtime_error ("No paths were found for flow " + std::to_string (flow.id));
          else if (paths.size () <= k)
            {
              randomlyRemoveExcessPaths = false;
              break;
            }
          else if (previousNumPathsFound == paths.size ())
            break;

          numPathsToGet++;
        }
      while (numbersAreClose (lastPathCost, secondToLastPathCost));

      if (randomlyRemoveExcessPaths == false)
        {
          AddDataPaths (flow, paths);
        }
      else
        { // Randomly choose which paths that have equal cost to the kth path to keep
          auto pathIterator = paths.begin ();
          std::advance (pathIterator, k - 1);
          auto kthPathCost = pathIterator->first;

          pathContainer_t finalPathSet;
          pathContainer_t pathsWithEqualCost;

          for (const auto &[pathCost, path] : paths)
            {
              if (pathCost < kthPathCost)
                finalPathSet.push_back (std::make_pair (pathCost, path));
              else if (numbersAreClose (pathCost, kthPathCost))
                pathsWithEqualCost.push_back (std::make_pair (pathCost, path));
            }

          if (finalPathSet.size () >= k)
            throw std::runtime_error ("Flow " + std::to_string (flowId) +
                                      " has more paths with lower cost than the kth path than "
                                      "expected");

          auto numPathsToSelectRandomly = k - finalPathSet.size ();

          pathContainer_t randomlyChosenPaths;
          std::sample (pathsWithEqualCost.begin (), pathsWithEqualCost.end (),
                       std::back_inserter (randomlyChosenPaths), numPathsToSelectRandomly,
                       std::mt19937{std::random_device{}()});

          finalPathSet.splice (finalPathSet.end (), randomlyChosenPaths);

          if (finalPathSet.size () != k)
            throw std::runtime_error ("Flow " + std::to_string (flowId) + " should have " +
                                      std::to_string (k) + " paths but it does not. It has " +
                                      std::to_string (finalPathSet.size ()) + " paths instead");

          AddDataPaths (flow, finalPathSet);
        }
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
BoostGraph::AddAckPaths (Flow::flowContainer_t &flows, const std::map<id_t, id_t> &oppositeLinkMap)
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

          for (const auto &linkId : path.GetLinks ())
            {
              LOG_MSG ("Working on link " << linkId);

              BoostGraph::link_t ackLink;

              if (oppositeLinkMap.find (linkId) != oppositeLinkMap.end ())
                {
                  auto oppositeLinkId = oppositeLinkMap.at (linkId);
                  ackLink = GetLink (oppositeLinkId);
                }
              else
                {
                  auto &dataLink = m_linkMap.at (linkId);

                  auto dataSrcNode = boost::source (dataLink, m_graph);
                  auto dataDstNode = boost::target (dataLink, m_graph);

                  bool ackLinkFound{false};

                  // The source and destination nodes are reversed to find the opposite
                  // link
                  std::tie (ackLink, ackLinkFound) =
                      boost::edge (dataDstNode, dataSrcNode, m_graph);

                  if (!ackLinkFound)
                    {
                      throw std::runtime_error ("The opposite link for link " +
                                                std::to_string (linkId) + " has not been found");
                    }
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

  pathContainer_t paths;
  auto numPathsFound = uint32_t{0};

  do
    {
      auto path =
          pathContainer_t{boost::yen_ksp (tempGraph, srcNode, dstNode,
                                          /* Link weight attribute */
                                          boost::get (&LinkDetails::cost, tempGraph),
                                          boost::get (boost::vertex_index_t (), tempGraph), 1)};

      if (path.empty ())
        break;

      paths.emplace_back (ConvertPath (tempGraph, path.front ()));

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

      numPathsFound++;
    }
  while (numPathsFound < k); // TODO: Verify that this is correct

  return paths;
}

/**
 * @brief Find the first K relaxed edge disjoint paths between a given source and destination node.
 *
 * The Relaxed Edge disjoint algorithm will find the first K edge disjoint
 * paths; however, different from the algorithm in FindKEdgeDisjointPaths, links
 * that are the only means of communication for that node are not deleted.
 *
 * The links that will not be deleted are discovered using the following method
 * after having found the shortest path that links the source with the
 * destination node. First, the shortest path is traversed from the source node,
 * to the destination nodes. For every node met, a check is made to see if it's
 * the only outgoing link available to that node. If it is, retain that link, if
 * not the link can be deleted in subsequent iterations. The same procedure is
 * repeated starting from the destination node and moving towards the source
 * node backwards. The link is retained if nodes only have one incoming link.
 *
 * @param srcNode The Source Node
 * @param dstNode The Destination Node
 * @param k The number of paths to find
 * @return The list of paths found
 */
BoostGraph::pathContainer_t
BoostGraph::GetKShortestRelaxedEdgeDisjointPaths (node_t srcNode, node_t dstNode, uint32_t k)
{
  /**
   * Find and save links that cannot be removed as they are the sole point of connection for the
   * particular source and destination node pair.
   */
  auto linksToRetain = std::set<id_t>{GetLinksToRetain (srcNode, dstNode)};

  graph_t tempGraph;
  boost::copy_graph (m_graph, tempGraph);

  pathContainer_t paths;
  auto numPathsFound = uint32_t{0};

  do
    {
      auto path =
          pathContainer_t{boost::yen_ksp (tempGraph, srcNode, dstNode,
                                          /* Link weight attribute */
                                          boost::get (&LinkDetails::cost, tempGraph),
                                          boost::get (boost::vertex_index_t (), tempGraph), 1)};

      if (path.empty ())
        break; // There are no more paths to find
      else if (!paths.empty () && PathsEqual (path.front ().second, paths.back ().second))
        break; // If the current and previously found path are the same, all paths have been found

      paths.emplace_back (ConvertPath (tempGraph, path.front ()));

      /**
       * Remove all the edges of the found path with the exception of the links
       * that are in the links to retain set.
       */
      for (const auto &[linkCost, links] : path)
        {
          for (const auto &link : links)
            {
              auto linkId = id_t{boost::get (&LinkDetails::id, tempGraph, link)};

              if (linksToRetain.find (linkId) != linksToRetain.end ())
                {
                  LOG_MSG ("Link: " << linkId << " Cost: " << linkCost << " has been retained");
                }
              else
                {
                  boost::remove_edge (link, tempGraph);
                  LOG_MSG ("Link: " << linkId << " Cost: " << linkCost << " has been removed");
                }
            }
        }

      numPathsFound++;
    }
  while (numPathsFound < k); // TODO: Verify that this is correct

  return paths;
}

std::set<id_t>
BoostGraph::GetLinksToRetain (node_t srcNode, node_t dstNode)
{
  auto path = boost::yen_ksp (m_graph, srcNode, dstNode,
                              /* Link weight attribute */ boost::get (&LinkDetails::cost, m_graph),
                              boost::get (boost::vertex_index_t (), m_graph), 1);

  if (path.empty ())
    throw std::runtime_error ("No path found");

  // Retrieve the shortest path
  const auto &shortestPath = path.front ().second;

  // Store the link ids that must not be removed
  std::set<id_t> linksToRetain;

  /**
   * Forward Search
   */
  LOG_MSG ("Starting forward search...");
  for (const auto &link : shortestPath)
    {
      auto srcNode{GetSourceNode (link)};
      // The number of outgoing links from a node excluding those connected to a terminal
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
  LOG_MSG ("Forward search complete");

  /**
   * Backward Search
   */
  LOG_MSG ("Starting backward search...");
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
  LOG_MSG ("Backward search complete");

  return linksToRetain;
}

BoostGraph::path_t
BoostGraph::ConvertPath (const graph_t &fromGraph, const path_t &fromPath)
{
  auto [pathCost, pathLinks] = fromPath;
  std::list<BoostGraph::link_t> convertedPathLinks;
  for (const auto &link : pathLinks)
    {
      auto linkId = id_t{boost::get (&LinkDetails::id, fromGraph, link)};
      convertedPathLinks.emplace_back (GetLink (linkId));
    }
  return std::make_pair (pathCost, convertedPathLinks);
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

bool
BoostGraph::PathsEqual (const std::list<BoostGraph::link_t> &pathA,
                        const std::list<BoostGraph::link_t> &pathB)
{
  if (pathA.size () != pathB.size ())
    return false;
  else // paths have equal size
    {
      auto itPathA = pathA.begin ();
      auto itPathB = pathB.begin ();

      while (itPathA != pathA.end () && itPathB != pathB.end ())
        {
          if (GetLinkId (*itPathA) != GetLinkId (*itPathB))
            return false;

          itPathA++;
          itPathB++;
        }
      return true;
    }
}
