#include "graph-manager.h"

GraphManager::GraphManager() : m_nodeType(m_graph), m_linkCapacity(m_graph), m_linkDelay(m_graph),
                               m_nodeCoordinates(m_graph), m_nodeShape(m_graph),
                               m_nodeColour(m_graph)
{}

void
GraphManager::ParseGraph(const std::string &lgfPath)
{
  try
    {
      lemon::digraphReader(m_graph, lgfPath). // Read the graph
        nodeMap("coordinates", m_nodeCoordinates).
        nodeMap("type", m_nodeType).
        arcMap("capacity", m_linkCapacity).
        arcMap("delay", m_linkDelay).
        run ();

#ifdef DEBUG
      std::cout << "Graph parsed successfully" << std::endl;
#endif
    }
  catch (lemon::Exception& e)
    {
      std::cerr << "Error parsing the LGF graph\n"
                << "LGF Location: "<< lgfPath << "\n"
                << "Error: " << e.what() << std::endl;
      throw;
    }
}
