/*
 * Path.h
 *
 *  Created on: Nov 28, 2018
 *      Author: noel
 */

#ifndef SRC_PATH_H_
#define SRC_PATH_H_

#include <tinyxml2.h>
#include <lemon/lp.h>

#include "definitions.h"

class Path
{
public:
  /**
   * Constructs a Path object from a Path XML element
   * @param pathElement The XML Path element
   */
  Path (tinyxml2::XMLElement* pathElement);

  /* Setters */
  void SetDataRateLpVar (lemon::Lp::Col lpVar) { m_assignedDataRate = lpVar; }

  /* Getters */
  id_t GetId () { return m_id; }
  double GetCost () { return m_cost; }
  lemon::Lp::Col GetDataRateLpVar () { return m_assignedDataRate; }

private:
  id_t m_id;
  double m_cost;
  lemon::Lp::Col m_assignedDataRate;
};

#endif /* SRC_PATH_H_ */
