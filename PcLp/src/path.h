/*
 * path.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_PATH_H_
#define SRC_PATH_H_

#include <vector>
#include <lemon/lp.h>
#include <tinyxml2.h>

#include "definitions.h"
#include "link.h"

class Link;

class Path
{
public:
  /**
   * Constructs a Path object from a Path XML element
   * @param pathElement The XML Path element
   */
  Path (tinyxml2::XMLElement *pathElement);

  void
  setDataRateLpVar (lemon::Lp::Col lpVar)
  {
    m_assignedDataRate = lpVar;
  }
  void
  addLink (Link *link)
  {
    m_links.push_back (link);
  }

  id_t
  getId ()
  {
    return m_id;
  }
  double
  getCost ()
  {
    return m_cost;
  }
  std::vector<Link *> &
  getLinks ()
  {
    return m_links;
  }
  lemon::Lp::Col
  getDataRateLpVar ()
  {
    return m_assignedDataRate;
  }

private:
  id_t m_id;
  double m_cost;
  std::vector<Link *> m_links;
  lemon::Lp::Col m_assignedDataRate;
};

#endif /* SRC_PATH_H_ */
