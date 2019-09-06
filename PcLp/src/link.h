/*
 * link.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_LINK_H_
#define SRC_LINK_H_

#include <tinyxml2.h>

#include "definitions.h"
#include "path.h"

class Path;

class Link
{
public:
  /**
   * Constructs a Link object from a Link XML element
   * @param linkElement The XML Link element
   */
  Link (tinyxml2::XMLElement *linkElement);

  void
  addPath (Path *path)
  {
    m_paths.push_back (path);
  }
  void
  addAckPath (Path *path)
  {
    m_AckPaths.push_back (path);
  }

  id_t
  getId () const
  {
    return m_id;
  }
  double
  getCost () const
  {
    return m_cost;
  }
  double
  getCapacity () const
  {
    return m_capacity;
  }

  std::vector<Path *> &
  getPaths ()
  {
    return m_paths;
  }
  std::vector<Path *> &
  getAckPaths ()
  {
    return m_AckPaths;
  }

private:
  id_t m_id;
  double m_cost;
  double m_capacity;
  std::vector<Path *> m_paths;
  std::vector<Path *> m_AckPaths;
};

#endif /* SRC_LINK_H_ */
