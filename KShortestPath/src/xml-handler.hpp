#ifndef xml_handler_hpp
#define xml_handler_hpp

#include <string>
#include <tinyxml2.h>

#include "lemon-graph.hpp"
#include "flow.hpp"

class XmlHandler
{
public:
  explicit XmlHandler ();

  void AddParameterList (const std::string& lgfPath, const uint32_t& k);
  void AddLinkDetails (const LemonGraph& lemonGraph);
  void AddFlows (const Flow::flowContainer_t& flows,
                 const LemonGraph& lemonGraph);
  void SaveXmlFile (const std::string& xmlFileLoc);

private:
  tinyxml2::XMLElement* CreateFlowElement (const Flow& flow);
  tinyxml2::XMLElement* CreatePathsElement (uint32_t& pathNumber,
                                            const std::vector<Path>& paths,
                                            const LemonGraph& lemonGraph);
  tinyxml2::XMLDocument m_xmlDoc;
  tinyxml2::XMLNode* m_rootNode;
};

#endif /* xml_handler_hpp */
