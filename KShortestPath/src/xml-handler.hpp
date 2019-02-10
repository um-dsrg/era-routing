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
    void AddParameterList (const std::string& inputFile, const std::string& outputFile,
                           const uint32_t globalK, bool perFlowK,
                           bool includeAllKEqualCostPaths);
    void AddLinkDetails(const BoostGraph& graph);
    void AddFlows(const Flow::flowContainer_t& flows);
    void AddNetworkTopology(const BoostGraph& graph);

    void SaveFile(const std::string& xmlFilePath);
private:
    tinyxml2::XMLElement* CreateFlowElement(const Flow& flow);
    tinyxml2::XMLElement* CreateDataPathsElement(const std::list<Path>& dataPaths);
    tinyxml2::XMLElement* CreateAckPathsElement(const std::list<Path>& ackPaths);
    tinyxml2::XMLElement* CreateLinkElement(const BoostGraph& graph, id_t linkId);
    
    tinyxml2::XMLDocument m_xmlDoc; /**< The XML Document. */
    tinyxml2::XMLNode* m_rootNode;  /**< Pointer to the XML root node. */
};

std::list<std::pair<id_t, id_t>> FindLinkPairs(const BoostGraph& graph);

#endif /* xml_handler_hpp */
