#ifndef boostgraph_hpp
#define boostgraph_hpp

#include <map>
#include <boost/graph/adjacency_list.hpp>

#include "flow.hpp"
#include "definitions.hpp"
#include "lemon-graph.hpp"

struct NodeDetails {
    id_t id;
    char type; /* S = Switch | T = Terminal */
};

struct LinkDetails {
    id_t id;
    linkCost_t cost;
    linkCapacity_t capacity;
};

class BoostGraph {
public:
    using graph_t = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                                          NodeDetails, LinkDetails>;
    using node_t = graph_t::vertex_descriptor;
    using link_t = graph_t::edge_descriptor;
    using pathContainer_t = std::list<std::pair<linkCost_t, std::list<BoostGraph::link_t>>>;
    
    explicit BoostGraph(const LemonGraph& lemonGraph);
    void FindKShortestPaths(Flow::flowContainer_t& flows, uint32_t k);
    void AddAckPaths(Flow::flowContainer_t& flows);

private:
    void GenerateBoostGraph(const LemonGraph& lemonGraph);
    void GenerateBoostNodes(const LemonGraph& lemonGraph);
    void GenerateBoostLinks(const LemonGraph& lemonGraph);
    
    void AddDataPaths(Flow& flow, const pathContainer_t& paths);
    
    graph_t m_graph;
    std::map<id_t, node_t> m_nodeMap;
    std::map<id_t, link_t> m_linkMap;
};

#endif /* boostgraph_hpp */
