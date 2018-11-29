/*
 * xml_handler.cc
 *
 *  Created on: Nov 29, 2018
 *      Author: noel
 */

#include <sstream>
#include "xml_handler.h"

XmlHandler::XmlHandler (std::string kspXmlPath)
{
  using namespace tinyxml2;
  XMLError eResult = m_kspXmlDoc.LoadFile(kspXmlPath.c_str());

  if (eResult != XML_SUCCESS)
    {
      std::stringstream ss;
      ss << "The file at: " << kspXmlPath << " could not be parsed";
      throw std::runtime_error(ss.str());
    }
}

tinyxml2::XMLNode*
XmlHandler::GetKspRootNode ()
{
  tinyxml2::XMLNode* rootNode = m_kspXmlDoc.FirstChildElement("Log");

  if (rootNode == nullptr)
    throw std::runtime_error("Could not find the root <Log> element in the given XML file");

  return rootNode;
}
