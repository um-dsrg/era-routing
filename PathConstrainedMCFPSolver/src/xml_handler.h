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

class XmlHandler
{
public:
  XmlHandler (std::string kspXmlPath);
  tinyxml2::XMLNode* GetKspRootNode ();

private:
  tinyxml2::XMLDocument m_kspXmlDoc;
};

#endif /* SRC_XML_HANDLER_H_ */
