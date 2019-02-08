#include <boost/numeric/conversion/cast.hpp>

#include "xml-handler.hpp"

using namespace tinyxml2;

/**
 Constructs the XML Document and creates the Log root element.
 */
XmlHandler::XmlHandler() {
    XMLNode* rootElement = m_xmlDoc.NewElement("Log");
    m_rootNode = m_xmlDoc.InsertFirstChild(rootElement);
    if (m_rootNode == nullptr) {
        throw std::runtime_error("Could not create element node");
    }
}

/**
 Log the parameters used to generate the result given in this XML result
 file.

 @param lgfPath The path to the LGF file used.
 @param k The number of paths to find for each flow.
 */
void XmlHandler::AddParameterList (const std::string &lgfPath, const uint32_t &k) {
    // TODO: This needs to be updated because K may be set separate for each flow
    XMLElement* parametersElement = m_xmlDoc.NewElement ("Parameters");

    XMLElement* lgfElement = m_xmlDoc.NewElement ("LgfFile");
    lgfElement->SetText (lgfPath.c_str ());
    parametersElement->InsertEndChild (lgfElement);

    XMLElement* kElement = m_xmlDoc.NewElement ("k");
    kElement->SetText (k);
    parametersElement->InsertEndChild (kElement);

    m_rootNode->InsertEndChild (parametersElement);
}


/**
 Adds a list of each link details to the XML result file.

 @param graph Instance of the Boost graph.
 */
void XmlHandler::AddLinkDetails(const BoostGraph &graph) {
    XMLElement* linkDetElement = m_xmlDoc.NewElement("LinkDetails");

    auto linkIterators = graph.GetLinkIterators();
    for (auto linkIt = linkIterators.first; linkIt != linkIterators.second; ++linkIt) {
        XMLElement* linkElement = m_xmlDoc.NewElement("Link");

        linkElement->SetAttribute("Id", graph.GetLinkId(*linkIt));
        linkElement->SetAttribute("Cost", graph.GetLinkCost(*linkIt));
        linkElement->SetAttribute("Capacity", graph.GetLinkCapacity(*linkIt));

        linkDetElement->InsertFirstChild(linkElement);
    }

    m_rootNode->InsertEndChild(linkDetElement);
}


void XmlHandler::AddFlows (const Flow::flowContainer_t& flows) {
    XMLElement* flowDetElement = m_xmlDoc.NewElement("FlowDetails");

    flowDetElement->SetAttribute("TotalNumFlows",
                                 boost::numeric_cast<uint32_t> (flows.size()));

    for (const auto& flowPair : flows) {
        XMLElement* flowElement {CreateFlowElement(flowPair.second)};
        flowDetElement->InsertEndChild(flowElement);
    }

    m_rootNode->InsertEndChild(flowDetElement);
}

XMLElement* XmlHandler::CreateFlowElement (const Flow &flow) {
    using boost::numeric_cast;
    XMLElement* flowElement = m_xmlDoc.NewElement("Flow");

    flowElement->SetAttribute("Id", flow.id);
    flowElement->SetAttribute("SourceNode", flow.sourceId);
    flowElement->SetAttribute("DestinationNode", flow.destinationId);
    flowElement->SetAttribute("DataRate", flow.dataRate);
    flowElement->SetAttribute("PacketSize",
                              boost::numeric_cast<uint32_t>(flow.packetSize));
    flowElement->SetAttribute("NumOfPakcets",
                              boost::numeric_cast<uint32_t>(flow.numOfPackets));
    flowElement->SetAttribute("Protocol", static_cast<char>(flow.protocol));
    flowElement->SetAttribute("StartTime",
                              boost::numeric_cast<uint32_t>(flow.startTime));
    flowElement->SetAttribute("EndTime",
                              boost::numeric_cast<uint32_t>(flow.endTime));

    flowElement->InsertEndChild(CreateDataPathsElement(flow.GetDataPaths()));
    flowElement->InsertEndChild(CreateAckPathsElement(flow.GetAckPaths()));

    return flowElement;
}

XMLElement* XmlHandler::CreateDataPathsElement(const std::list<Path>& dataPaths) {
    XMLElement* pathsElement = m_xmlDoc.NewElement("Paths");
    pathsElement->SetAttribute("NumPaths", boost::numeric_cast<uint32_t>(dataPaths.size()));

    for (const auto& path : dataPaths) {
        XMLElement* pathElement = m_xmlDoc.NewElement("Path");
        pathElement->SetAttribute("Id", path.id);
        pathElement->SetAttribute("Cost", path.cost);

        for (const auto& linkId : path.GetLinks()) {
            XMLElement* linkElement = m_xmlDoc.NewElement("Link");
            linkElement->SetAttribute("Id", linkId);
            pathElement->InsertEndChild(linkElement);
        }

        pathsElement->InsertEndChild(pathElement);
    }

    return pathsElement;
}

XMLElement* XmlHandler::CreateAckPathsElement(const std::list<Path>& ackPaths) {
    XMLElement* pathsElement = m_xmlDoc.NewElement("AckPaths");
    pathsElement->SetAttribute("NumPaths", boost::numeric_cast<uint32_t>(ackPaths.size()));

    for (const auto& path : ackPaths) {
        XMLElement* pathElement = m_xmlDoc.NewElement("Path");
        pathElement->SetAttribute("Id", path.id);
        pathElement->SetAttribute("Cost", path.cost);

        for (const auto& linkId : path.GetLinks()) {
            XMLElement* linkElement = m_xmlDoc.NewElement("Link");
            linkElement->SetAttribute("Id", linkId);
            pathElement->InsertEndChild(linkElement);
        }

        pathsElement->InsertEndChild(pathElement);
    }

    return pathsElement;
}

void XmlHandler::SaveFile(const std::string& xmlFilePath) {
    if (m_xmlDoc.SaveFile(xmlFilePath.c_str()) != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Could not save XML File in " + xmlFilePath);
    }
}
//
//XMLElement* create_link_element(tinyxml2::XMLDocument& xml_document,
//                                lemon::SmartDigraph::Arc& link,
//                                LemonGraph& lemon_graph)
//{
//    DefLemonGraph::graph_t& graph = lemon_graph.GetGraph();
//    lemon::SmartDigraph::Node sourceNode = graph.source(link);
//    lemon::SmartDigraph::Node destinationNode = graph.target(link);
//    int sourceNodeId = lemon_graph.GetNodeId(sourceNode);
//    int destinationNodeId = lemon_graph.GetNodeId(destinationNode);
//
//    XMLElement* linkElement = xml_document.NewElement("LinkElement");
//
//    linkElement->SetAttribute("Id", graph.id(link));
//    linkElement->SetAttribute("SourceNode", sourceNodeId);
//    linkElement->SetAttribute("SourceNodeType",
//                              std::string(1, lemon_graph.GetNodeType(sourceNode)).c_str());
//    linkElement->SetAttribute("DestinationNode", destinationNodeId);
//    linkElement->SetAttribute("DestinationNodeType",
//                              std::string(1, lemon_graph.GetNodeType(destinationNode)).c_str());
//    linkElement->SetAttribute("Capacity", lemon_graph.GetLinkCapacity(link));
//
//    return linkElement;
//}
//
//void XmlHandler::add_network_topology(LemonGraph &lemon_graph)
//{
//    XMLElement* net_top_element = m_xmlDoc.NewElement("NetworkTopology");
//    auto num_terminals = lemon_graph.get_num_terminals();
//    auto num_switches = lemon_graph.get_num_switches();
//
//    net_top_element->SetAttribute("NumberOfTerminals", num_terminals);
//    net_top_element->SetAttribute("NumberOfSwitches", num_switches);
//    net_top_element->SetAttribute("NumberOfLinks", lemon::countArcs(lemon_graph.GetGraph()));
//
//    std::set<int> visitedLinks;
//
//    // We need to loop through all the links and add their details.
//    for (auto link = lemon_graph.GetLinkIt(); link != lemon::INVALID; ++ link)
//    {
//        int linkId( lemon_graph.GetLinkId(link) );
//        int oppositeLinkId{0};
//
//        // This link is already stored in the XML file. Skip it.
//        if (visitedLinks.find(linkId) != visitedLinks.end()) continue;
//
//        lemon::SmartDigraph::Arc oppositeLink;
//
//        double currentLinkDelay{ lemon_graph.GetLinkCost(link) };
//        bool pairFound(false);
//
//        // Check if link with opposite source and destination exists.
//        for (lemon::ConArcIt<lemon::SmartDigraph> oppositeLinkIt(lemon_graph.GetGraph(), lemon_graph.GetGraph().target(link), lemon_graph.GetGraph().source(link));
//            oppositeLinkIt != lemon::INVALID; ++oppositeLinkIt)
//        {
//            oppositeLinkId = lemon_graph.GetLinkId(oppositeLinkIt);
//            // This link is already stored in the XML file. Skip it.
//            if (visitedLinks.find(oppositeLinkId) != visitedLinks.end()) continue;
//
//            //lemon::SmartDigraph::ArcMap<double> m_linkDelay;
//            if (currentLinkDelay == lemon_graph.GetLinkCost(oppositeLinkIt))
//            {
//                pairFound = true;
//                oppositeLink = oppositeLinkIt;
//                break;
//            }
//        }
//
//        if (!pairFound) // If no pair was found issue a warning.
//        {
//            std::cerr << "Warning: Link " << linkId << " has no opposite link." << std::endl;
//            visitedLinks.insert(linkId);
//
//            XMLElement* linkElement = m_xmlDoc.NewElement("Link");
//            linkElement->SetAttribute("Delay", lemon_graph.GetLinkCost(link));
//            linkElement->InsertFirstChild(create_link_element(m_xmlDoc, link, lemon_graph));
//
//            net_top_element->InsertFirstChild(linkElement);
//        }
//        else // A pair is found.
//        {
//            // Verify that the links have the same cost
//            link_t link = lemon_graph.GetLink(linkId);
//            link_t opposite_link = lemon_graph.GetLink(oppositeLinkId);
//            assert(lemon_graph.GetLinkCost(link) == lemon_graph.GetLinkCost(opposite_link));
//
//            visitedLinks.insert(linkId);
//            visitedLinks.insert(oppositeLinkId);
//
//            XMLElement* linkElement = m_xmlDoc.NewElement("Link");
//            linkElement->SetAttribute("Delay", lemon_graph.GetLinkCost(link));
//            linkElement->InsertFirstChild(create_link_element(m_xmlDoc, link, lemon_graph));
//            linkElement->InsertFirstChild(create_link_element(m_xmlDoc, oppositeLink, lemon_graph));
//
//            net_top_element->InsertFirstChild(linkElement);
//        }
//    }
//
//    // Add a comment in the XML file that will describe the units being used.
//    XMLComment* comment = m_xmlDoc.NewComment("Delay (ms), Capacity (Mbps), Node Type (T=Terminal, S=Switch)");
//    net_top_element->InsertFirstChild(comment);
//    m_rootNode->InsertEndChild(net_top_element);
//}
//

