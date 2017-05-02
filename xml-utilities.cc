#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>

#include "xml-utilities.h"

using namespace tinyxml2;

XMLNode*
XmlUtilities::GetRootNode(XMLDocument& xmlDoc)
{
  return xmlDoc.FirstChild();
}

XMLNode*
XmlUtilities::InsertRootNode (XMLDocument& xmlDoc,
                              const std::string &rootElementName)
{
  XMLNode* rootElement = xmlDoc.NewElement(rootElementName.c_str());
  return xmlDoc.InsertFirstChild(rootElement);
}

void
XmlUtilities::SaveXmlFile(const std::string& fileFullPath, tinyxml2::XMLDocument& xmlDoc,
                          bool insertDeclaration)
{
  try
    {
      // Insert declaration before saving
      if (insertDeclaration)  xmlDoc.InsertFirstChild(xmlDoc.NewDeclaration());
      if (xmlDoc.SaveFile(fileFullPath.c_str()) != XML_SUCCESS)
        {
          throw std::ios_base::failure("Could not save the XML Log File");
        }
    }
  catch (const std::ios_base::failure& e)
    {
      std::cerr << e.what() << std::endl;
      throw;
    }
}

void
XmlUtilities::InsertTimeStampInRootElement (XMLDocument& xmlDoc)
{
  std::chrono::time_point<std::chrono::system_clock> currentTime (std::chrono::system_clock::now());
  std::time_t currentTimeFormatted = std::chrono::system_clock::to_time_t(currentTime);

  std::stringstream ss;
  ss << std::put_time(std::localtime(&currentTimeFormatted), "%a %d-%m-%Y %T");

  XMLElement* rootElement = XmlUtilities::GetRootNode(xmlDoc)->ToElement();
  rootElement->SetAttribute("Generated", ss.str().c_str());
}
