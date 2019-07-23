/*
 * xml_handler.h
 *
 *  Created on: Nov 29, 2018
 *      Author: noel
 */

#ifndef SRC_XML_HANDLER_H_
#define SRC_XML_HANDLER_H_

#include <string>
#include <tinyxml2.h>

#include "definitions.h"
#include "lp_solver.h"

class XmlHandler
{
public:
  XmlHandler (std::string pathsXmlFile);
  tinyxml2::XMLNode* getKspRootNode ();
  void saveResults (linkContainer_t& links, pathContainer_t& paths, flowContainer_t& flows,
                    LpSolver& lpSolver, std::string resultXmlPath);

private:
  tinyxml2::XMLDocument m_kspXmlDoc;
};

#endif /* SRC_XML_HANDLER_H_ */
