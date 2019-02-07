#include <lemon/lgf_reader.h>

#include "lemon-graph.hpp"

LemonGraph::LemonGraph (const std::string& lgfPath) :
    m_nodeLabel(m_graph), m_nodeType(m_graph), m_nodeCoordinates(m_graph),
    m_linkLabel(m_graph), m_linkCost(m_graph), m_linkCapacity(m_graph)
{
    LoadGraphFromFile(lgfPath);
}

LemonGraph::node_t LemonGraph::GetNode (id_t nodeId) const {
    try {
        return m_nodeIdToNode.at(nodeId);
    } catch (const std::out_of_range& ex) {
        throw std::out_of_range("Node with id: " + std::to_string(nodeId) + " not found");
    }
}

id_t LemonGraph::GetNodeId(LemonGraph::node_t node) const {
    return m_nodeLabel[node];
}

char LemonGraph::GetNodeType(node_t node) const {
    return m_nodeType[node];
}

LemonGraph::nodeIt_t LemonGraph::GetNodeIt() const {
    return nodeIt_t(m_graph);
}

LemonGraph::link_t LemonGraph::GetLink(id_t linkId) const {
    try {
        return m_linkIdToLink.at (linkId);
    } catch (const std::out_of_range& ex) {
        throw std::out_of_range("Link with id: " + std::to_string(linkId) + "not found");
    }
}

id_t LemonGraph::GetLinkId(LemonGraph::link_t link) const {
    return m_linkLabel[link];
}

linkCost_t LemonGraph::GetLinkCost(LemonGraph::link_t link) const {
    return m_linkCost[link];
}

linkCapacity_t LemonGraph::GetLinkCapacity(link_t link) const {
    return m_linkCapacity[link];
}

LemonGraph::linkIt_t LemonGraph::GetLinkIt() const {
    return LemonGraph::linkIt_t(m_graph);
}

LemonGraph::node_t LemonGraph::GetSourceNode(link_t link) const {
    return m_graph.source (link);
}

LemonGraph::node_t LemonGraph::GetDestinationNode(link_t link) const {
    return m_graph.target (link);
}

/**
 * @brief Loads the LGF graph into the m_lGraph instance
 * @param lgfPath The path to the LGF file
 */
void LemonGraph::LoadGraphFromFile(const std::string& lgfPath) {
    try {
        lemon::digraphReader(m_graph, lgfPath)
            .nodeMap("label", m_nodeLabel)
            .nodeMap("coordinates", m_nodeCoordinates)
            .nodeMap("type", m_nodeType)
            .arcMap("label", m_linkLabel)
            .arcMap("delay", m_linkCost)
            .arcMap("capacity", m_linkCapacity)
            .run();
        
        LOG_MSG("Graph parsed successfully");
        
        LOG_MSG("Building node id -> node map...");
        for (nodeIt_t node = GetNodeIt(); node != lemon::INVALID; ++node) {
            identifier_t nodeId = m_nodeLabel[node];
            m_nodeIdToNode.emplace(nodeId, node);
        }
        
        LOG_MSG("Building link id -> link map...");
        for (linkIt_t link = GetLinkIt(); link != lemon::INVALID; ++link) {
            identifier_t linkId = m_linkLabel[link];
            m_linkIdToLink.emplace(linkId, link);
        }
    } catch (const lemon::Exception& e) {
        throw std::runtime_error("Error parsing the LGS graph.\nLGF Location: " + lgfPath +
                                 "\nError: " + e.what());
    }
}
