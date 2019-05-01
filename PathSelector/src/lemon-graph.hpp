#ifndef lemon_graph_hpp
#define lemon_graph_hpp

#include <map>
#include <lemon/dim2.h>
#include <lemon/smart_graph.h>

#include "definitions.hpp"

class LemonGraph
{
public:
  using graph_t = lemon::SmartDigraph;
  /* Node definitions */
  using node_t = graph_t::Node;
  using nodeIt_t = graph_t::NodeIt;
  using nodeLabelMap_t = graph_t::NodeMap<id_t>;
  using nodeTypeMap_t = graph_t::NodeMap<char>;
  using nodeCoordMap_t = graph_t::NodeMap<lemon::dim2::Point<int>>;
  /* Link definitions */
  using link_t = graph_t::Arc;
  using linkIt_t = graph_t::ArcIt;
  using linkLabelMap_t = graph_t::ArcMap<id_t>;
  using linkCostMap_t = graph_t::ArcMap<linkCost_t>;
  using linkCapacityMap_t = graph_t::ArcMap<linkCapacity_t>;

  explicit LemonGraph (const std::string &lgfPath);

  /* Node getters */
  id_t GetNodeId (node_t node) const;
  char GetNodeType (node_t node) const;
  nodeIt_t GetNodeIt () const;

  /* Link getters */
  id_t GetLinkId (link_t link) const;
  linkCost_t GetLinkCost (link_t link) const;
  linkCapacity_t GetLinkCapacity (link_t link) const;
  linkIt_t GetLinkIt () const;
  node_t GetSourceNode (link_t link) const;
  node_t GetDestinationNode (link_t link) const;

private:
  void LoadGraphFromFile (const std::string &lgfPath);

  graph_t m_graph; /**< The Lemon graph. */

  /* Node Maps */
  nodeLabelMap_t m_nodeLabel; /**< Maps the node label (id) with the node. */
  nodeTypeMap_t m_nodeType; /**< Maps the node type with the node. */
  nodeCoordMap_t m_nodeCoordinates; /**< Maps the node coordinates with the node. */

  /* Link Maps */
  linkLabelMap_t m_linkLabel; /**< Maps the link label (id) with the link. */
  linkCostMap_t m_linkCost; /**< Maps the link cost with the link. */
  linkCapacityMap_t m_linkCapacity; /**< Maps the link capacity with the link. */
};

#endif /* lemon_graph_hpp */
