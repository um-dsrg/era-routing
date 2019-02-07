#include <boost/numeric/conversion/cast.hpp>

#include "yen_ksp.hpp"
#include "boost-graph.hpp"

BoostGraph::BoostGraph (const LemonGraph& lemonGraph) {
    GenerateBoostGraph(lemonGraph);
}

/**
 * @brief Generate a boost graph equivalent of the lemon graph
 * @param lemonGraph LemonGraph instance that contains the parsed lemon graph
 */
void BoostGraph::GenerateBoostGraph(const LemonGraph& lemonGraph) {
    GenerateBoostNodes(lemonGraph);
    GenerateBoostLinks(lemonGraph);
    LOG_MSG("LEMON Graph converted to BOOST Graph successfully");
}

void BoostGraph::GenerateBoostNodes(const LemonGraph& lemonGraph) {
    LOG_MSG("Building nodes...");
    for (auto lemonNode = lemonGraph.GetNodeIt(); lemonNode != lemon::INVALID; ++lemonNode) {
        auto nodeId {lemonGraph.GetNodeId(lemonNode)};
        auto nodeType {lemonGraph.GetNodeType(lemonNode)};
        
        auto boostNode {boost::add_vertex({nodeId, nodeType}, m_graph)};
        auto ret = m_nodeMap.emplace(nodeId, boostNode);
        if (!ret.second) {
            throw std::runtime_error("Trying to insert a duplicate node. Node Id: " +
                                     std::to_string(nodeId) + "\n");
        }
        
        LOG_MSG("Added node " << boost::get(&NodeDetails::id, m_graph, boostNode) <<
                " Type " << boost::get(&NodeDetails::type, m_graph, boostNode));
    }
}

void BoostGraph::GenerateBoostLinks(const LemonGraph& lemonGraph) {
    LOG_MSG("Building links...");
    for (auto lemonLink = lemonGraph.GetLinkIt(); lemonLink != lemon::INVALID; ++lemonLink) {
        auto srcNodeId = lemonGraph.GetNodeId(lemonGraph.GetSourceNode(lemonLink));
        auto dstNodeId = lemonGraph.GetNodeId(lemonGraph.GetDestinationNode(lemonLink));
        
        auto linkId = lemonGraph.GetLinkId(lemonLink);
        auto linkCost = lemonGraph.GetLinkCost(lemonLink);
        auto linkCapacity = lemonGraph.GetLinkCapacity(lemonLink);
        
        link_t boostLink;
        bool linkAdded {false};
        std::tie(boostLink, linkAdded) = boost::add_edge(m_nodeMap.at(srcNodeId),
                                                         m_nodeMap.at(dstNodeId),
                                                         {linkId, linkCost, linkCapacity},
                                                         m_graph);
        
        if (!linkAdded) {
            throw std::runtime_error("Link could not be added in Boost graph. Link Id: "
                                     + std::to_string(linkId) + "\n");
        }
        
        auto srcNode {boost::source(boostLink, m_graph)};
        auto dstNode {boost::target(boostLink, m_graph)};
        
        LOG_MSG("Added link " << boost::get(&LinkDetails::id, m_graph, boostLink) <<
                " Cost " << boost::get(&LinkDetails::cost, m_graph, boostLink) <<
                " Capacity " << boost::get(&LinkDetails::capacity, m_graph, boostLink) <<
                " Source Node " << boost::get(&NodeDetails::id, m_graph, srcNode) <<
                " Destination Node " << boost::get(&NodeDetails::id, m_graph, dstNode));
    }
}

//
//bool numbersAreClose(linkCapacity_t value1, linkCapacity_t value2,
//                     linkCapacity_t accuracy=1e-9)
//{
//  return (std::fabs (value1 - value2) < accuracy);
//}
//
///**
// * @brief Add the kth shortest paths for each data flow including paths with
// * same cost as path k
// *
// * Add the kth shortest paths for each data flow including paths with same
// * cost as path k. Data flows refer to either TCP or UDP flows. Acknowledgement
// * flows are routed over the single shortest path (i.e. k = 1).
// *
// * @param flows List of flows that will be updated with the paths
// * @param k The number of paths to include for each flow
// */
//void
//BoostGraph::AddKShortestPaths(Flow::flowContainer_t& flows, uint32_t k)
//{
//  for (auto& flowPair: flows)
//    {
//      Flow &flow = flowPair.second;
//
//      DefBoostGraph::node_t srcNode =
//        GetNode (m_lemonGraph.GetNode (flow.GetSourceId ()));
//      DefBoostGraph::node_t dstNode =
//          GetNode (m_lemonGraph.GetNode (flow.GetDestinationId ()));
//
//      std::list<std::pair<linkCost_t,
//                          std::list<DefBoostGraph::link_t>>> kShortestPaths;
//
//      if (flow.GetProtocol () == Protocol::Ack)  // Route Ack flows
//        {
//          kShortestPaths = boost::yen_ksp (m_graph, srcNode, dstNode, 1);
//
//          if (kShortestPaths.empty ())
//            throw std::runtime_error ("ACK flow route not found");
//        }
//      else // Route data flows
//        {
//          uint64_t numPathsToInclude {0};
//          uint32_t increasedK{k};
//          std::list<DefBoostGraph::link_t> prevLastPathLinks;
//
//          while (true)
//            {
//              increasedK =
//                boost::numeric_cast<uint32_t> (std::lround (increasedK * 1.5));
//
//              kShortestPaths = boost::yen_ksp (m_graph, srcNode, dstNode,
//                                               increasedK);
//
//              if (kShortestPaths.size () < k)
//                { // Number of paths found less than requested
//                  numPathsToInclude = kShortestPaths.size ();
//                  break;
//                }
//
//              auto& lastPath = kShortestPaths.back ();
//
//              if (lastPath.second == prevLastPathLinks) // No new paths for flow
//                {
//                  numPathsToInclude = kShortestPaths.size();
//                  break;
//                }
//              else
//                {
//                  auto kthPath = kShortestPaths.begin ();
//                  std::advance (kthPath, k - 1);
//                  linkCost_t kthPathCost{kthPath->first};
//
//                  numPathsToInclude = k;
//
//                  for (auto& currentPath = ++kthPath; // Start from path k+1
//                       currentPath != kShortestPaths.end ();
//                       ++currentPath)
//                    {
//                      if (numbersAreClose (kthPathCost, currentPath->first))
//                        numPathsToInclude++;
//                    }
//
//                  if (numPathsToInclude < kShortestPaths.size())
//                    break;
//                  else if (numPathsToInclude > kShortestPaths.size())
//                    {
//                      std::stringstream ss;
//                      ss << "Error when finding paths for flow: "
//                         << flow.GetFlowId ();
//                      throw std::runtime_error(ss.str ());
//                    }
//                }
//              prevLastPathLinks = std::move(kShortestPaths.back().second);
//            }
//
//          auto firstPathToRemove = kShortestPaths.begin();
//          std::advance (firstPathToRemove, numPathsToInclude);
//          kShortestPaths.erase(firstPathToRemove, kShortestPaths.end ());
//        }
//
//      for (auto& path: kShortestPaths) // Add all the paths to the flow
//          flow.AddPath(Path(path, m_blLinkMap));
//    }
//}
