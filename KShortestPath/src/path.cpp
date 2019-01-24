#include "path.hpp"

Path::Path(std::pair<linkCost_t, std::list<DefBoostGraph::link_t>> path,
           const std::map<DefBoostGraph::link_t,
                          DefLemonGraph::link_t>& m_blLinkMap) :
    pathCost (path.first)
{
  for (auto& link : path.second) // Populate list of links the path uses
    links.emplace_back (m_blLinkMap.at (link));
}

/**
 * @brief Overloads the Path::operator = to check if to Path objects are equal
 */
Path&
Path::operator= ( const Path& rhs )
{
  if (&rhs == this) return *this; // Check for self-assignment

  pathCost = rhs.pathCost;
  links = rhs.links;
  return *this;
}

/**
 * @brief Overloads the Path::operator < to compare to paths. Used for sorting
 */
bool
Path::operator< (const Path& other) const
{
  return pathCost < other.pathCost;
}

std::string Path::str (const LemonGraph &lemonGraph)
{
  std::stringstream ss;

  ss << "Path Cost: " << pathCost << "\n"
     << "Path: ";

  for (auto& link : links)
    ss << lemonGraph.GetLinkId (link) << " ";

  return ss.str();
}
