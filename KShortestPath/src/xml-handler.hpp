#ifndef xml_handler_hpp
#define xml_handler_hpp

#include <list>
#include <string>
#include <tinyxml2.h>

#include "flow.hpp"
#include "boost-graph.hpp"

class XmlHandler {
public:
    explicit XmlHandler();
    void AddParameterList(const std::string& lgfPath, const uint32_t& k);
    void AddLinkDetails(const BoostGraph& graph);
    void AddFlows(const Flow::flowContainer_t& flows);

    void SaveFile(const std::string& xmlFilePath);
private:
    tinyxml2::XMLElement* CreateFlowElement(const Flow& flow);
    tinyxml2::XMLElement* CreateDataPathsElement(const std::list<Path>& dataPaths);
    tinyxml2::XMLElement* CreateAckPathsElement(const std::list<Path>& ackPaths);

    tinyxml2::XMLDocument m_xmlDoc;
    tinyxml2::XMLNode* m_rootNode;
};

//  void AddFlows (const Flow::flowContainer_t& flows, const LemonGraph& lemonGraph);
//  void add_network_topology(LemonGraph& lemon_graph);
//
//private:
//  tinyxml2::XMLElement* CreateFlowElement (const Flow& flow);
//  tinyxml2::XMLElement* CreatePathsElement (uint32_t& pathNumber,
//                                            const std::vector<Path>& paths,
//                                            const LemonGraph& lemonGraph);
//  tinyxml2::XMLDocument m_xmlDoc;
//  tinyxml2::XMLNode* m_rootNode;
//};

#endif /* xml_handler_hpp */
