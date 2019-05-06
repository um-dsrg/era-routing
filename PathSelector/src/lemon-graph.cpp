#include <lemon/lgf_reader.h>

#include "lemon-graph.hpp"

/**
 Build a LemonGraph from the given LGF file.

 @param lgfPath The path to the LGF file.
 */
LemonGraph::LemonGraph (const std::string &lgfPath)
    : m_nodeLabel (m_graph),
      m_nodeType (m_graph),
      m_nodeCoordinates (m_graph),
      m_linkLabel (m_graph),
      m_linkCost (m_graph),
      m_linkCapacity (m_graph)
{
  LoadGraphFromFile (lgfPath);
}

/**
 Retrieve the node id given a Lemon graph node.

 @param node The Lemon graph node.
 @return The node id.
 */
id_t
LemonGraph::GetNodeId (LemonGraph::node_t node) const
{
  return m_nodeLabel[node];
}

/**
 Retrieve the node type given a Lemon graph node.

 @param node The Lemon graph node.
 @return The node type.
 */
char
LemonGraph::GetNodeType (node_t node) const
{
  return m_nodeType[node];
}

/**
 Returns a node iterator.

 @return Lemon node iterator.
 */
LemonGraph::nodeIt_t
LemonGraph::GetNodeIt () const
{
  return nodeIt_t (m_graph);
}

/**
 Retrieve the link id given a Lemon graph link.

 @param link The Lemon graph link.
 @return The link id.
 */
id_t
LemonGraph::GetLinkId (LemonGraph::link_t link) const
{
  return m_linkLabel[link];
}

/**
 Retrieve the link cost given a Lemon graph link.

 @param link The Lemon graph link.
 @return The link cost.
 */
linkCost_t
LemonGraph::GetLinkCost (LemonGraph::link_t link) const
{
  return m_linkCost[link];
}

/**
 Retrieve the link capacity given a Lemon graph link.

 @param link The Lemon graph link.
 @return The link capacity.
 */
linkCapacity_t
LemonGraph::GetLinkCapacity (link_t link) const
{
  return m_linkCapacity[link];
}

/**
 Returns a link iterator.

 @return Lemon link iterator.
 */

LemonGraph::linkIt_t
LemonGraph::GetLinkIt () const
{
  return LemonGraph::linkIt_t (m_graph);
}

/**
 Retrieve the source node of a given Lemon graph link.

 @param link The Lemon graph link.
 @return The source node of the given link.
 */
LemonGraph::node_t
LemonGraph::GetSourceNode (link_t link) const
{
  return m_graph.source (link);
}

/**
 Retrieve the destination node of a given Lemon graph link.

 @param link The Lemon graph link.
 @return The destination node of the given link.
 */
LemonGraph::node_t
LemonGraph::GetDestinationNode (link_t link) const
{
  return m_graph.target (link);
}

/**
 Build the Lemon graph from the given LGF file.

 @param lgfPath The path to the LGF file.
 */
void
LemonGraph::LoadGraphFromFile (const std::string &lgfPath)
{
  try
    {
      lemon::digraphReader (m_graph, lgfPath)
          .nodeMap ("label", m_nodeLabel)
          .nodeMap ("coordinates", m_nodeCoordinates)
          .nodeMap ("type", m_nodeType)
          .arcMap ("label", m_linkLabel)
          .arcMap ("delay", m_linkCost)
          .arcMap ("capacity", m_linkCapacity)
          .run ();

      LOG_MSG ("Graph parsed successfully");
  } catch (const lemon::Exception &e)
    {
      throw std::runtime_error ("Error parsing the LGF graph.\nLGF Location: " + lgfPath +
                                "\nError: " + e.what ());
  }
}
