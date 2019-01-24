#ifndef BOOSTGRAPH_HPP
#define BOOSTGRAPH_HPP

#include <map>

#include "definitions.hpp"
#include "lemon-graph.hpp"
#include "flow.hpp"

class BoostGraph
{
public:
  explicit BoostGraph (const LemonGraph& lemonGraph);

  DefBoostGraph::node_t GetNode (DefLemonGraph::node_t lemonNode);

  void GenerateBoostGraphFromLemonGraph (const LemonGraph& lemonGraph);
  void AddKShortestPaths (Flow::flowContainer_t &flows, uint32_t k);

private:
  DefBoostGraph::graph_t m_graph;
  // Mapping between Boost <-> Lemon
  std::map<DefLemonGraph::node_t, DefBoostGraph::node_t> m_lbNodeMap;
  std::map<DefBoostGraph::link_t, DefLemonGraph::link_t> m_blLinkMap;

  const LemonGraph& m_lemonGraph;
};

#endif /* BOOSTGRAPH_HPP */
