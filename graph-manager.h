#ifndef GRAPH_MANAGER_H
#define GRAPH_MANAGER_H

#include <lemon/smart_graph.h>
#include <lemon/lgf_reader.h>
#include <lemon/lgf_writer.h>
#include <lemon/graph_to_eps.h>

class GraphManager
{
public:
  GraphManager();

  void ParseGraph (const std::string& lgfPath);
private:
  lemon::SmartDigraph m_graph;
  lemon::SmartDigraph::NodeMap<char> m_nodeType;
  lemon::SmartDigraph::ArcMap<double> m_linkCapacity;
  lemon::SmartDigraph::ArcMap<double> m_linkDelay;

  // Storing the node's coordinates
  lemon::SmartDigraph::NodeMap<lemon::dim2::Point<int>> m_nodeCoordinates;
  // Storing the node's shape
  lemon::SmartDigraph::NodeMap<int> m_nodeShape; /*!< CIRCLE = 0, SQUARE = 1, DIAMOND = 2 */
  // Storing the node's colour
  lemon::SmartDigraph::NodeMap<lemon::Color> m_nodeColour;
};

#endif /* GRAPH_MANAGER_H */
