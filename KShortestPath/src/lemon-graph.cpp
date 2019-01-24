#include <lemon/lgf_reader.h>

#include "lemon-graph.hpp"

LemonGraph::LemonGraph () : m_nodeLabel(m_graph), m_nodeType(m_graph),
                            m_nodeCoordinates(m_graph),
                            m_linkLabel(m_graph), m_linkCost(m_graph),
                            m_linkCapacity(m_graph)
{}

DefLemonGraph::node_t
LemonGraph::GetNode (identifier_t nodeId) const
{
  try
    {
      return m_nodeIdToNode.at (nodeId);
    }
  catch (std::out_of_range& ex)
    {
      std::stringstream ss;
      ss << "Node with id: " << nodeId << " not found";
      throw std::out_of_range(ss.str ());
    }
}

DefLemonGraph::link_t
LemonGraph::GetLink (identifier_t linkId) const
{
  try
    {
      return m_linkIdToLink.at (linkId);
    }
  catch (std::out_of_range& ex)
    {
      std::stringstream ss;
      ss << "Link with id: " << linkId << " not found";
      throw std::out_of_range(ss.str ());
    }
}

uint32_t LemonGraph::get_num_switches()
{
    uint32_t num_switches {0};

    for (auto const& node_entry: m_nodeIdToNode) {
        auto& node = node_entry.second;
        auto node_type = m_nodeType[node];
        if (node_type == 'S')
            num_switches++;
    }

    return num_switches;
}

uint32_t LemonGraph::get_num_terminals()
{
    uint32_t num_terminals {0};

    for (auto const& node_entry: m_nodeIdToNode) {
        auto& node = node_entry.second;
        auto node_type = m_nodeType[node];
        if (node_type == 'T')
            num_terminals++;
    }

    return num_terminals;
}

/**
 * @brief Loads the LGF graph into the m_lGraph instance
 * @param lgfPath The path to the LGF file
 */
void
LemonGraph::LoadGraphFromFile(const std::string& lgfPath)
{
  try
    {
      lemon::digraphReader(m_graph, lgfPath)
        .nodeMap ("label", m_nodeLabel)
        .nodeMap ("coordinates", m_nodeCoordinates)
        .nodeMap ("type", m_nodeType)
        .arcMap ("label", m_linkLabel)
        .arcMap ("delay", m_linkCost)
        .arcMap ("capacity", m_linkCapacity)
        .run();

      LOG_MSG("Graph parsed successfully");

      LOG_MSG("Building node id -> node map...");
      for (nodeIt_t node = GetNodeIt (); node != lemon::INVALID; ++node)
        {
          identifier_t nodeId = m_nodeLabel[node];
          m_nodeIdToNode.insert (std::make_pair(nodeId, node));
        }

      LOG_MSG("Building link id -> link map...");
      for (linkIt_t link = GetLinkIt (); link != lemon::INVALID; ++link)
        {
          identifier_t linkId = m_linkLabel[link];
          m_linkIdToLink.insert (std::make_pair(linkId, link));
        }
    }
  catch (lemon::Exception& e)
    {
      std::stringstream ss;
      ss << "Error parsing the LGF graph\nLGF Location: "
         << lgfPath << "\n" << "Error: " << e.what();
      throw std::runtime_error(ss.str());
    }
}
