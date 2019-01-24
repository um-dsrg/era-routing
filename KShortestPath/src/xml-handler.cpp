#include <boost/numeric/conversion/cast.hpp>

#include "xml-handler.hpp"

using namespace tinyxml2;

/**
 * @brief Constructs the XML object by creating a Log element as the root
 */
XmlHandler::XmlHandler ()
{
  XMLNode* rootElement = m_xmlDoc.NewElement("Log");
  m_rootNode = m_xmlDoc.InsertFirstChild(rootElement);
  if (m_rootNode == nullptr)
    throw std::runtime_error("Could not create element node");
}
/**
 * @brief Adds the parameters used to run this program to the XML file
 * @param lgfPath Path to the LGF file
 * @param k k's value
 */
void
XmlHandler::AddParameterList (const std::string &lgfPath, const uint32_t &k)
{
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
 * @brief Adds the link details to the XML file
 * @param lemonGraph Instance of the LemonGraph object
 */
void
XmlHandler::AddLinkDetails (const LemonGraph &lemonGraph)
{
  XMLElement* linkDetailsElement = m_xmlDoc.NewElement ("LinkDetails");
  linkDetailsElement->SetAttribute ("NumberOfLinks",
                                    lemonGraph.GetNumLinks ());

  for (DefLemonGraph::linkIt_t link = lemonGraph.GetLinkIt ();
       link != lemon::INVALID;
       ++link)
    {
      XMLElement* linkElement = m_xmlDoc.NewElement ("Link");

      linkElement->
        SetAttribute ("Id",
                      boost::numeric_cast<uint32_t>
                          (lemonGraph.GetLinkId (link)));
      linkElement->SetAttribute ("Cost", lemonGraph.GetLinkCost (link));
      linkElement->SetAttribute ("Capacity", lemonGraph.GetLinkCapacity (link));

      linkDetailsElement->InsertFirstChild (linkElement);
    }

  m_rootNode->InsertEndChild (linkDetailsElement);
}
/**
 * @brief Add flows to the XML file
 * @param flows List of flows
 * @param lemonGraph Instance of the LemonGraph class
 */
void
XmlHandler::AddFlows (const Flow::flowContainer_t &flows,
                      const LemonGraph& lemonGraph)
{
  using boost::numeric_cast;
  XMLElement* flowDetailsElement = m_xmlDoc.NewElement("FlowDetails");
  flowDetailsElement->SetAttribute ("TotalNumFlows",
                                    numeric_cast<uint32_t> (flows.size()));

  uint32_t pathNumber = 0;

  // Save data flows BEFORE acknowledgement flows. This is required by the
  // Genetic Algorithm to simplify the network matrix operations.
  for (const auto& flowPair : flows)
    {
      const Flow& flow = flowPair.second;
      if (flow.GetProtocol () == Protocol::Ack) continue; // Skip ACK flows

      XMLElement* flowElement {CreateFlowElement (flow)};
      XMLElement* pathsElement {CreatePathsElement (pathNumber,
                                                    flow.GetPaths (),
                                                    lemonGraph)};
      flowElement->InsertEndChild (pathsElement);
      flowDetailsElement->InsertEndChild (flowElement);
    }

  for (const auto& flowPair : flows)
    {
      const Flow& flow = flowPair.second;
      if (flow.GetProtocol () != Protocol::Ack) continue; // Save ACK flows ONLY

      XMLElement* flowElement {CreateFlowElement (flow)};
      XMLElement* pathsElement {CreatePathsElement (pathNumber,
                                                    flow.GetPaths (),
                                                    lemonGraph)};
      flowElement->InsertEndChild (pathsElement);
      flowDetailsElement->InsertEndChild (flowElement);
    }

  flowDetailsElement->SetAttribute ("TotalNumPaths", pathNumber);
  m_rootNode->InsertEndChild (flowDetailsElement);
}
/**
 * @brief Saves the XML file in memory to a file
 * @param xmlFileLoc The full path where to save the xml file
 */
void
XmlHandler::SaveXmlFile(const std::string &xmlFileLoc)
{
  if (m_xmlDoc.SaveFile(xmlFileLoc.c_str()) != tinyxml2::XML_SUCCESS)
    throw std::runtime_error("Could not save XML File");
}
/**
 * @brief Creates a Flow element from a flow object
 * @param flow A flow object
 * @return Returns a pointer to the created Flow element
 */
tinyxml2::XMLElement*
XmlHandler::CreateFlowElement (const Flow &flow)
{
  using boost::numeric_cast;
  XMLElement* flowElement = m_xmlDoc.NewElement("Flow");

  flowElement->SetAttribute ("Id",
                             numeric_cast<uint32_t> (flow.GetFlowId ()));
  flowElement->SetAttribute ("SourceNode",
                             numeric_cast<uint32_t> (flow.GetSourceId ()));
  flowElement->SetAttribute ("DestinationNode",
                             numeric_cast<uint32_t> (flow.GetDestinationId ()));
  flowElement->SetAttribute ("DataRate", flow.GetDataRate ());
  flowElement->SetAttribute ("PacketSize",
                             numeric_cast<uint32_t> (flow.GetPktSize ()));
  flowElement->SetAttribute ("NumOfPackets",
                             numeric_cast<uint32_t> (flow.GetNumOfPackets ()));
  flowElement->
    SetAttribute ("Protocol",
                  std::string (1, static_cast<char>
                                  (flow.GetProtocol ())).c_str ());
  flowElement->SetAttribute ("StartTime",
                             numeric_cast<uint32_t> (flow.GetStartTime ()));
  flowElement->SetAttribute ("EndTime",
                             numeric_cast<uint32_t> (flow.GetEndTime ()));

  // Port numbers
  if (flow.GetProtocol () == Protocol::Tcp)
    {
      flowElement->SetAttribute ("SrcPortNumber",
                                 numeric_cast<uint32_t> (flow.GetSrcPort ()));
      flowElement->SetAttribute ("DstPortNumber",
                                 numeric_cast<uint32_t> (flow.GetDstPort ()));
    }
  else
    flowElement->SetAttribute ("PortNumber",
                               numeric_cast<uint32_t> (flow.GetDstPort ()));

  if (flow.GetProtocol () == Protocol::Ack)
    flowElement->SetAttribute ("TcpFlowId",
                               numeric_cast<uint32_t> (flow.GetTcpFlowId ()));

  return flowElement;
}
/**
 * @brief Creates the paths element
 * @param pathNumber A path counter used to give each path a unique Id
 * @param paths List of paths the flow has
 * @param lemonGraph Reference to the LemonGraph class
 * @return Returns the created paths element
 */
tinyxml2::XMLElement*
XmlHandler::CreatePathsElement (uint32_t& pathNumber,
                                const std::vector<Path> &paths,
                                const LemonGraph& lemonGraph)
{
  using boost::numeric_cast;

  XMLElement* pathsElement = m_xmlDoc.NewElement("Paths");
  pathsElement->SetAttribute ("NumPaths",
                              numeric_cast<uint32_t> (paths.size ()));

  for (const auto& path: paths)
    {
      XMLElement *pathElement = m_xmlDoc.NewElement ("Path");
      pathElement->SetAttribute ("Id", pathNumber++);
      pathElement->SetAttribute ("Cost", path.pathCost);

      for (const auto& link: path.links)
        {
          XMLElement* linkElement = m_xmlDoc.NewElement ("Link");
          linkElement->SetAttribute ("Id",
            numeric_cast <uint32_t> (lemonGraph.GetLinkId (link)));

          pathElement->InsertEndChild (linkElement);
        }

      pathsElement->InsertEndChild (pathElement);
    }

  return pathsElement;
}
