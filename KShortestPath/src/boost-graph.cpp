#include <list>
#include <math.h>
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

        auto ret = m_linkMap.emplace(linkId, boostLink);
        if (!ret.second) {
            throw std::runtime_error("Failed to insert link " + std::to_string(linkId) + " in the link map");
        }

#ifdef MY_DEBUG /* Debug only Logging */
        auto srcNode {boost::source(boostLink, m_graph)};
        auto dstNode {boost::target(boostLink, m_graph)};
#endif
        LOG_MSG("Added link " << boost::get(&LinkDetails::id, m_graph, boostLink) <<
                " Cost " << boost::get(&LinkDetails::cost, m_graph, boostLink) <<
                " Capacity " << boost::get(&LinkDetails::capacity, m_graph, boostLink) <<
                " Source Node " << boost::get(&NodeDetails::id, m_graph, srcNode) <<
                " Destination Node " << boost::get(&NodeDetails::id, m_graph, dstNode));
    }
}

BoostGraph::link_t BoostGraph::GetLink(id_t linkId) const {
    try {
        return m_linkMap.at(linkId);
    } catch (const std::out_of_range& oor) {
        std::cerr << "The link " << linkId << " was not found" << std::endl;
        throw;
    }
}

id_t BoostGraph::GetLinkId(const BoostGraph::link_t& link) const {
    return boost::get(&LinkDetails::id, m_graph, link);
}

linkCost_t BoostGraph::GetLinkCost(const BoostGraph::link_t& link) const {
    return boost::get(&LinkDetails::cost, m_graph, link);
}

linkCapacity_t BoostGraph::GetLinkCapacity(const BoostGraph::link_t& link) const {
    return boost::get(&LinkDetails::capacity, m_graph, link);
}

id_t BoostGraph::GetOppositeLink(id_t linkId) const {
    auto link = GetLink(linkId);
    auto linkCost {GetLinkCost(link)};

    auto srcNode {boost::source(link, m_graph)};
    auto dstNode {boost::target(link, m_graph)};
    auto dstNodeId {GetNodeId(dstNode)};

    auto oppositeLinkId = id_t{linkId};
    auto incomingLinksIterators {boost::in_edges(srcNode, m_graph)};

    for (auto incomingLinkIt = incomingLinksIterators.first;
         incomingLinkIt != incomingLinksIterators.second;
         ++incomingLinkIt) {
        auto incomingLinkSrcNodeId = GetNodeId(boost::source(*incomingLinkIt, m_graph));
        auto incomingLinkCost = GetLinkCost(*incomingLinkIt);

        if ((incomingLinkSrcNodeId == dstNodeId) && (linkCost == incomingLinkCost)) {
            oppositeLinkId = incomingLinkSrcNodeId;
            break;
        }
    }

    if (linkId == oppositeLinkId) {
        std::cout << "Warning: Link " << linkId << " has no opposite link" << std::endl;
    }

    return oppositeLinkId;
}

std::pair<BoostGraph::graph_t::edge_iterator,
          BoostGraph::graph_t::edge_iterator> BoostGraph::GetLinkIterators() const {
    return boost::edges(m_graph);
}

id_t BoostGraph::GetNodeId(const BoostGraph::node_t& node) const {
    return boost::get(&NodeDetails::id, m_graph, node);
}

char BoostGraph::GetNodeType(const node_t node) const {
    return boost::get(&NodeDetails::type, m_graph, node);
}

BoostGraph::node_t BoostGraph::GetSourceNode(const link_t& link) const {
    return boost::source(link, m_graph);
}

BoostGraph::node_t BoostGraph::GetDestinationNode(const link_t& link) const {
    return boost::target(link, m_graph);
}

bool numbersAreClose(double value1, double value2, double accuracy=1e-9) {
    return (std::fabs (value1 - value2) < accuracy);
}

/**
 @brief Add the kth shortest path for each data flow based on the flow's k value

 Add the kth shortest paths for each data flow. If includeAllKEqualCostPaths is
 set include all the paths with the same cost as path k. If k is equal to 1, then
 only one path will be chosen to simulate OSPF routing.

 Acknowledgement paths take the reverse route of the corresponding data path.

 @param flows The flows that will be updated with the paths
 @param includeAllKEqualCostPaths Flag that when set allows the KSP algorithm to
                                  include all the paths with cost equal to the kth
                                  path.
 */
void BoostGraph::FindKShortestPaths(Flow::flowContainer_t& flows, bool includeAllKEqualCostPaths) {
    for (auto& flowPair : flows) {
        auto& flow {flowPair.second};

        auto k = flow.k;
        auto& srcNode {m_nodeMap.at(flow.sourceId)};
        auto& dstNode {m_nodeMap.at(flow.destinationId)};
        
        auto kShortestPaths = pathContainer_t{boost::yen_ksp(m_graph, srcNode, dstNode,
                                                             /* Link weight attribute */ boost::get(&LinkDetails::cost, m_graph),
                                                             boost::get(boost::vertex_index_t(), m_graph), k)};
        
        if (kShortestPaths.empty()) {
            throw std::runtime_error("No paths were found for flow " + std::to_string(flow.id));
        } else if ( k != 1 && includeAllKEqualCostPaths && (kShortestPaths.size() == k)) {
            // Only search for more paths if K is not equal to 1, the includeAllEqualCostPaths is enabled, and if the number
            // of found paths is equal to k; thus, we need more paths to determine whether all paths have been included.
            auto kthPathCost {kShortestPaths.back().first};
            auto allEqualCostPathsFound = bool{false};
            auto extendedK = uint32_t{k};
            
            while (allEqualCostPathsFound == false) {
                extendedK = boost::numeric_cast<uint32_t>(std::ceil(extendedK * 1.5));
                kShortestPaths = boost::yen_ksp(m_graph, srcNode, dstNode,
                                                boost::get(&LinkDetails::cost, m_graph),
                                                boost::get(boost::vertex_index_t(), m_graph), extendedK);
                
                if (numbersAreClose(kShortestPaths.back().first, kthPathCost)) {
                    continue; // The last path cost is equal to the K shortest path. Need to increase K even further.
                } else {
                    allEqualCostPathsFound = true;
                    // Remove all paths that have a cost larger than the kthPathCost
                    kShortestPaths.remove_if([kthPathCost](const std::pair<linkCost_t, std::list<BoostGraph::link_t>>& path) -> bool {
                        if (path.first < kthPathCost || numbersAreClose(path.first, kthPathCost)) {
                            return true;
                        } else {
                            return false;
                        }
                    });
                }
            }
        }
        
        AddDataPaths(flow, kShortestPaths);
    }
}

void BoostGraph::AddDataPaths(Flow& flow, const BoostGraph::pathContainer_t& paths) {
    for (const auto& path: paths) {
        Path dataPath(/* assign a path id to this path */ true);
        dataPath.cost = path.first;
        
        for (const auto& link : path.second) {
            dataPath.AddLink(boost::get(&LinkDetails::id, m_graph, link));
        }
        flow.AddDataPath(dataPath);
    }
}

void BoostGraph::AddAckPaths(Flow::flowContainer_t& flows) {
    for (auto& flowPair : flows) {
        auto& flow {flowPair.second};

        for (const auto& path : flow.GetDataPaths()) {
            Path ackPath(/* do not assign a path id to this path */ false);
            ackPath.id = path.id; // Set the Ack Path id to be identical to the Data path id

            for (const auto& link : path.GetLinks()) {
                std::cout << "We are working on link " << link << std::endl;
                auto& dataLink = m_linkMap.at(link);

                auto dataSrcNode = boost::source(dataLink, m_graph);
                auto dataDstNode = boost::target(dataLink, m_graph);

                bool ackLinkFound {false};
                BoostGraph::link_t ackLink;

                // The source and destination nodes are reversed to find the opposite link
                std::tie(ackLink, ackLinkFound) = boost::edge(dataDstNode, dataSrcNode, m_graph);

                if (!ackLinkFound) {
                    throw std::runtime_error("The opposite link for link " + std::to_string(link) +
                                             " has not been found");
                }
                ackPath.AddLink(boost::get(&LinkDetails::id, m_graph, ackLink));
            }

            flow.AddAckPath(ackPath);
        }
    }
}
