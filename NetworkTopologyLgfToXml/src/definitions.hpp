#ifndef definitions_hpp
#define definitions_hpp

#include <cstdint>

// General Definitions
using identifier_t = uint64_t;
using linkCost_t = double;
using linkCapacity_t = double;

// Boost Definitions
#include <boost/graph/adjacency_list.hpp>
namespace DefBoostGraph
{
    using graph_t = boost::adjacency_list<boost::vecS, boost::vecS,
        boost::directedS,
        /* Vertex properties */ boost::no_property,
        /* Edge properties   */ boost::property<boost::edge_weight_t,
        linkCost_t> >;
    using node_t = graph_t::vertex_descriptor;
    using link_t = graph_t::edge_descriptor;
}

// Lemon Definitions
#include <lemon/smart_graph.h>
#include <lemon/dim2.h>

namespace DefLemonGraph
{
    using graph_t = lemon::SmartDigraph;
    //// Node related items
    using node_t = graph_t::Node;
    using nodeIt_t = graph_t::NodeIt;
    using nodeLabelMap_t = graph_t::NodeMap<identifier_t>;
    using nodeTypeMap_t = graph_t::NodeMap<char>;
    using nodeCoordMap_t = graph_t::NodeMap<lemon::dim2::Point<int>>;
    //// Link related items
    using link_t = graph_t::Arc;
    using linkIt_t = graph_t::ArcIt;
    using linkLabelMap_t = graph_t::ArcMap<identifier_t>;
    using linkCostMap_t = graph_t::ArcMap<linkCost_t>;
    using linkCapacityMap_t = graph_t::ArcMap<linkCapacity_t>;
}

#ifdef MY_DEBUG
#define LOG_MSG(x) do { std::cerr << x << std::endl; } while (0)
#else
#define LOG_MSG(x)
#endif

#endif /* definitions_hpp */
