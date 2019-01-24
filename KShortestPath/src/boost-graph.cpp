#include <boost/numeric/conversion/cast.hpp>
#include "boost-graph.hpp"
#include "yen_ksp.hpp"

BoostGraph::BoostGraph (const LemonGraph& lemonGraph): m_lemonGraph(lemonGraph)
{}

/**
 * @brief Returns the boost graph's lemon node equivalent
 * @param lemonNode The node in the Lemon graph
 * @return The Boost node represented by the lemon's graph lemon node lemonNode
 */
DefBoostGraph::node_t
BoostGraph::GetNode(DefLemonGraph::node_t lemonNode)
{
  try
    {
      return m_lbNodeMap.at (lemonNode);
    }
  catch (std::out_of_range&)
    {
      throw std::out_of_range ("Lemon node not found in boost graph\n");
    }
}

/**
 * @brief Generate a boost graph equivalent of the lemon graph
 * @param lemonGraph LemonGraph instance that contains the parsed lemon graph
 */
void
BoostGraph::GenerateBoostGraphFromLemonGraph(const LemonGraph& lemonGraph)
{
  LOG_MSG("Building nodes...");

  for (DefLemonGraph::nodeIt_t lemonNode = lemonGraph.GetNodeIt ();
       lemonNode != lemon::INVALID;
       ++lemonNode)
    {
      DefBoostGraph::node_t node = boost::add_vertex(m_graph);

      auto ret = m_lbNodeMap.insert(std::make_pair(lemonNode, node));
      if (!ret.second)
        {
          std::stringstream ss;
          ss << "Trying to insert a duplicate node. Node Id: "
             << lemonGraph.GetNodeId (lemonNode) << "\n";
          throw std::runtime_error(ss.str());
        }
    }

  LOG_MSG("Building links...");

  for (DefLemonGraph::linkIt_t lemonLink = lemonGraph.GetLinkIt ();
       lemonLink != lemon::INVALID;
       ++lemonLink)
    {
      DefLemonGraph::node_t lemonSrcNode = lemonGraph.GetSourceNode (lemonLink);
      DefLemonGraph::node_t lemonDstNode =
          lemonGraph.GetDestinationNode (lemonLink);

      // Get boost source and destination nodes
      DefBoostGraph::node_t srcNode = GetNode (lemonSrcNode);
      DefBoostGraph::node_t dstNode = GetNode (lemonDstNode);

      // Create link
      DefBoostGraph::link_t link;
      bool linkAdded;
      boost::tie (link, linkAdded) = boost::add_edge(srcNode, dstNode, m_graph);

      if (!linkAdded)
        {
          std::stringstream ss;
          ss << "Link could not be added in Boost graph. Link Id: "
             << lemonGraph.GetLinkId (lemonLink) << "\n";
          throw std::runtime_error(ss.str());
        }

      // Set the cost of the link
      boost::get(boost::edge_weight, m_graph, link) =
          lemonGraph.GetLinkCost (lemonLink);

      // Add boost -> lemon link map
      auto ret = m_blLinkMap.insert(std::make_pair(link, lemonLink));
      if (!ret.second)
        {
          std::stringstream ss;
          ss << "Trying to insert a duplicate link. Link Id: "
             << lemonGraph.GetLinkId (lemonLink) << "\n";
          throw std::runtime_error(ss.str());
        }
    }

  LOG_MSG("LEMON Graph converted to BOOST Graph successfully");
}

bool numbersAreClose(linkCapacity_t value1, linkCapacity_t value2,
                     linkCapacity_t accuracy=1e-9)
{
  return (std::fabs (value1 - value2) < accuracy);
}

/**
 * @brief Add the kth shortest paths for each data flow including paths with
 * same cost as path k
 *
 * Add the kth shortest paths for each data flow including paths with same
 * cost as path k. Data flows refer to either TCP or UDP flows. Acknowledgement
 * flows are routed over the single shortest path (i.e. k = 1).
 *
 * @param flows List of flows that will be updated with the paths
 * @param k The number of paths to include for each flow
 */
void
BoostGraph::AddKShortestPaths(Flow::flowContainer_t& flows, uint32_t k)
{
  for (auto& flowPair: flows)
    {
      Flow &flow = flowPair.second;

      DefBoostGraph::node_t srcNode =
        GetNode (m_lemonGraph.GetNode (flow.GetSourceId ()));
      DefBoostGraph::node_t dstNode =
          GetNode (m_lemonGraph.GetNode (flow.GetDestinationId ()));

      std::list<std::pair<linkCost_t,
                          std::list<DefBoostGraph::link_t>>> kShortestPaths;

      if (flow.GetProtocol () == Protocol::Ack)  // Route Ack flows
        {
          kShortestPaths = boost::yen_ksp (m_graph, srcNode, dstNode, 1);

          if (kShortestPaths.empty ())
            throw std::runtime_error ("ACK flow route not found");
        }
      else // Route data flows
        {
          uint64_t numPathsToInclude {0};
          uint32_t increasedK{k};
          std::list<DefBoostGraph::link_t> prevLastPathLinks;

          while (true)
            {
              increasedK =
                boost::numeric_cast<uint32_t> (std::lround (increasedK * 1.5));

              kShortestPaths = boost::yen_ksp (m_graph, srcNode, dstNode,
                                               increasedK);

              if (kShortestPaths.size () < k)
                { // Number of paths found less than requested
                  numPathsToInclude = kShortestPaths.size ();
                  break;
                }

              auto& lastPath = kShortestPaths.back ();

              if (lastPath.second == prevLastPathLinks) // No new paths for flow
                {
                  numPathsToInclude = kShortestPaths.size();
                  break;
                }
              else
                {
                  auto kthPath = kShortestPaths.begin ();
                  std::advance (kthPath, k - 1);
                  linkCost_t kthPathCost{kthPath->first};

                  numPathsToInclude = k;

                  for (auto& currentPath = ++kthPath; // Start from path k+1
                       currentPath != kShortestPaths.end ();
                       ++currentPath)
                    {
                      if (numbersAreClose (kthPathCost, currentPath->first))
                        numPathsToInclude++;
                    }

                  if (numPathsToInclude < kShortestPaths.size())
                    break;
                  else if (numPathsToInclude > kShortestPaths.size())
                    {
                      std::stringstream ss;
                      ss << "Error when finding paths for flow: "
                         << flow.GetFlowId ();
                      throw std::runtime_error(ss.str ());
                    }
                }
              prevLastPathLinks = std::move(kShortestPaths.back().second);
            }

          auto firstPathToRemove = kShortestPaths.begin();
          std::advance (firstPathToRemove, numPathsToInclude);
          kShortestPaths.erase(firstPathToRemove, kShortestPaths.end ());
        }

      for (auto& path: kShortestPaths) // Add all the paths to the flow
          flow.AddPath(Path(path, m_blLinkMap));
    }
}
