#ifndef LEMONGRAPH_HPP
#define LEMONGRAPH_HPP

#include <map>

#include "definitions.hpp"

using namespace DefLemonGraph;
/**
 * @brief The LemonGraph class
 */
class LemonGraph
{
public:
    LemonGraph();

    /* Node related items*/
    node_t GetNode(identifier_t nodeId) const; /*{ return m_nodeIdToNode.at(nodeId); }*/
    identifier_t GetNodeId(node_t node) const { return m_nodeLabel[node]; }
    char GetNodeType(node_t node) const { return m_nodeType[node]; }
    nodeIt_t GetNodeIt() const { return nodeIt_t(m_graph); }
    
    /* Link related items*/
    link_t GetLink(identifier_t linkId) const;
    identifier_t GetLinkId(link_t link) const { return m_linkLabel[link]; }
    linkCost_t GetLinkCost(link_t link) const { return m_linkCost[link]; }
    linkCapacity_t GetLinkCapacity(link_t link) const
    {
        return m_linkCapacity[link];
    }
    uint32_t GetNumLinks() const
    {
        return static_cast<uint32_t>(m_graph.arcNum());
    }
    node_t GetSourceNode(link_t link) const { return m_graph.source(link); }
    node_t GetDestinationNode(link_t link) const
    {
        return m_graph.target(link);
    }
    linkIt_t GetLinkIt() const { return linkIt_t(m_graph); }

    graph_t& GetGraph() { return m_graph; }

    /* Functions */
    void LoadGraphFromFile(const std::string& lgfPath);

private:
    graph_t m_graph;
    /* Node Maps */
    nodeLabelMap_t m_nodeLabel;
    nodeTypeMap_t m_nodeType;
    nodeCoordMap_t m_nodeCoordinates;
    std::map<identifier_t, node_t> m_nodeIdToNode;
    /* Link Maps */
    linkLabelMap_t m_linkLabel;
    linkCostMap_t m_linkCost;
    linkCapacityMap_t m_linkCapacity;
    std::map<identifier_t, link_t> m_linkIdToLink;
};

#endif // LEMONGRAPH_HPP
