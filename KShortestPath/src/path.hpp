#ifndef PATH_HPP
#define PATH_HPP

#include "definitions.hpp"
#include "lemon-graph.hpp"

class Path
{
public:
  Path(std::pair<linkCost_t, std::list<DefBoostGraph::link_t>> path,
       const std::map<DefBoostGraph::link_t,
       DefLemonGraph::link_t>& m_blLinkMap);

  Path& operator= (const Path& rhs);
  bool operator< (const Path& other) const;

  std::string str(const LemonGraph& lemonGraph);

  linkCost_t pathCost; // Path cost
  std::vector<DefLemonGraph::link_t> links; // List of links the path goes through
};

#endif // PATH_HPP
