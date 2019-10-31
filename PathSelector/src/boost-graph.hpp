#ifndef boostgraph_hpp
#define boostgraph_hpp

#include <map>
#include <set>
#include <boost/graph/adjacency_list.hpp>

#include "flow.hpp"
#include "definitions.hpp"
#include "lemon-graph.hpp"

/**
 Represents all the details associated with a boost graph node.
 */
struct NodeDetails
{
  id_t id;
  char type; /* S = Switch | T = Terminal */
};

/**
 Represents all the details associated with a boost graph link.
 */
struct LinkDetails
{
  id_t id;
  linkCost_t cost;
  linkCapacity_t capacity;
};

class BoostGraph
{
public:
  using graph_t = boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                                        NodeDetails, LinkDetails>;
  using node_t = graph_t::vertex_descriptor;
  using link_t = graph_t::edge_descriptor;
  using path_t = std::pair<linkCost_t, std::list<BoostGraph::link_t>>;
  using pathContainer_t = std::list<path_t>;

  explicit BoostGraph (const LemonGraph &lemonGraph);

  link_t GetLink (id_t linkId) const;
  id_t GetLinkId (const link_t &link) const;
  linkCost_t GetLinkCost (const link_t &link) const;
  linkCapacity_t GetLinkCapacity (const link_t &link) const;
  std::list<id_t> GetOppositeLink (id_t linkId) const;
  std::pair<graph_t::edge_iterator, graph_t::edge_iterator> GetLinkIterators () const;

  id_t GetNodeId (const node_t &node) const;
  char GetNodeType (node_t node) const;
  node_t GetSourceNode (const link_t &link) const;
  node_t GetDestinationNode (const link_t &link) const;

  void AssignPathsToFlows (Flow::flowContainer_t &flows, const std::string &pathSelectionAlgorithm);
  void AddAckPaths (Flow::flowContainer_t &flows);
  void AddShortestPathAck (Flow::flowContainer_t &flows);

private:
  void GenerateBoostGraph (const LemonGraph &lemonGraph);
  void GenerateBoostNodes (const LemonGraph &lemonGraph);
  void GenerateBoostLinks (const LemonGraph &lemonGraph);

  pathContainer_t GetKShortestPaths (node_t srcNode, node_t dstNode, uint32_t k);
  pathContainer_t GetKShortestEdgeDisjointPaths (node_t srcNode, node_t dstNode, uint32_t k);
  pathContainer_t GetKShortestRelaxedEdgeDisjointPaths (node_t srcNode, node_t dstNode, uint32_t k);
  std::set<id_t> GetLinksToRetain (node_t srcNode, node_t dstNode);
  path_t ConvertPath (const graph_t &fromGraph, const path_t &path);

  void AddDataPaths (Flow &flow, const pathContainer_t &paths);
  bool PathsEqual (const std::list<BoostGraph::link_t> &pathA,
                   const std::list<BoostGraph::link_t> &pathB);

  graph_t m_graph; /**< The boost graph. */
  std::map<id_t, node_t> m_nodeMap; /**< Maps the node id with its respective boost node. */
  std::map<id_t, link_t> m_linkMap; /**< Maps the link id with its respective boost link. */
};

#endif /* boostgraph_hpp */
