#include <boost/numeric/conversion/cast.hpp>

#include "xml-handler.hpp"

using namespace tinyxml2;

/**
 Creates an XML Document and creates the Log root element.
 */
XmlHandler::XmlHandler ()
{
  XMLNode *rootElement = m_xmlDoc.NewElement ("Log");
  m_rootNode = m_xmlDoc.InsertFirstChild (rootElement);
  if (m_rootNode == nullptr)
    {
      throw std::runtime_error ("Could not create element node");
    }
}

/**
 Add the paramters used in the result file.

 @param inputFile The path to the LGF input file.
 @param outputFile The output path where to store the result.
 @param globalK The value of the \p globalK variable.
 @param perFlowK The value of the \p perFlowK flag.
 @param includeAllKEqualCostPaths The value of the \p includeAllKEqualCostPaths variable.
 */
void
XmlHandler::AddParameterList (const std::string &inputFile, const std::string &outputFile,
                              const uint32_t globalK, bool perFlowK, bool includeAllKEqualCostPaths)
{
  XMLElement *parametersElement = m_xmlDoc.NewElement ("Parameters");

  XMLElement *inputFileElement = m_xmlDoc.NewElement ("InputFile");
  inputFileElement->SetText (inputFile.c_str ());
  parametersElement->InsertEndChild (inputFileElement);

  XMLElement *outputFileElement = m_xmlDoc.NewElement ("OutputFile");
  outputFileElement->SetText (outputFile.c_str ());
  parametersElement->InsertEndChild (outputFileElement);

  XMLElement *globalKElement = m_xmlDoc.NewElement ("GlobalK");
  globalKElement->SetText (globalK);
  parametersElement->InsertEndChild (globalKElement);

  XMLElement *perFlowKElement = m_xmlDoc.NewElement ("PerFlowK");
  if (perFlowK)
    {
      perFlowKElement->SetText ("Enabled");
    }
  else
    {
      perFlowKElement->SetText ("Disabled");
    }
  parametersElement->InsertEndChild (perFlowKElement);

  XMLElement *includeAllPathsElement = m_xmlDoc.NewElement ("includeAllKEqualCostPaths");
  if (includeAllKEqualCostPaths)
    {
      includeAllPathsElement->SetText ("Enabled");
    }
  else
    {
      includeAllPathsElement->SetText ("Disabled");
    }
  parametersElement->InsertEndChild (includeAllPathsElement);

  m_rootNode->InsertEndChild (parametersElement);
}

/**
 Adds a list of each link details to the XML result file.

 @param graph Instance of the Boost graph.
 */
void
XmlHandler::AddLinkDetails (const BoostGraph &graph)
{
  XMLElement *linkDetElement = m_xmlDoc.NewElement ("LinkDetails");

  auto linkIterators = graph.GetLinkIterators ();
  for (auto linkIt = linkIterators.first; linkIt != linkIterators.second; ++linkIt)
    {
      XMLElement *linkElement = m_xmlDoc.NewElement ("Link");

      linkElement->SetAttribute ("Id", graph.GetLinkId (*linkIt));
      linkElement->SetAttribute ("Cost", graph.GetLinkCost (*linkIt));
      linkElement->SetAttribute ("Capacity", graph.GetLinkCapacity (*linkIt));

      linkDetElement->InsertFirstChild (linkElement);
    }

  m_rootNode->InsertEndChild (linkDetElement);
}

/**
 Add the flow details, including each of their paths to the XML result file.

 @param flows The flows container.
 */
void
XmlHandler::AddFlows (const Flow::flowContainer_t &flows)
{
  XMLElement *flowDetElement = m_xmlDoc.NewElement ("FlowDetails");

  flowDetElement->SetAttribute ("TotalNumFlows", boost::numeric_cast<uint32_t> (flows.size ()));

  for (const auto &flowPair : flows)
    {
      XMLElement *flowElement{CreateFlowElement (flowPair.second)};
      flowDetElement->InsertEndChild (flowElement);
    }

  m_rootNode->InsertEndChild (flowDetElement);
}

/**
 Generate an XML Flow element given a Flow object.

 @param flow The flow object to generate an element.
 @return The created XML element.
 */
XMLElement *
XmlHandler::CreateFlowElement (const Flow &flow)
{
  using boost::numeric_cast;
  XMLElement *flowElement = m_xmlDoc.NewElement ("Flow");

  flowElement->SetAttribute ("Id", flow.id);
  flowElement->SetAttribute ("SourceNode", flow.sourceId);
  flowElement->SetAttribute ("DestinationNode", flow.destinationId);
  flowElement->SetAttribute ("RequestedDataRate", flow.dataRate);
  flowElement->SetAttribute ("PacketSize", boost::numeric_cast<uint32_t> (flow.packetSize));
  flowElement->SetAttribute ("NumOfPakcets", boost::numeric_cast<uint32_t> (flow.numOfPackets));
  std::string flowProtocol{static_cast<char> (flow.protocol)};
  flowElement->SetAttribute ("Protocol", flowProtocol.c_str ());
  flowElement->SetAttribute ("StartTime", boost::numeric_cast<uint32_t> (flow.startTime));
  flowElement->SetAttribute ("EndTime", boost::numeric_cast<uint32_t> (flow.endTime));
  flowElement->SetAttribute ("k", flow.k);

  flowElement->InsertEndChild (CreateDataPathsElement (flow.GetDataPaths ()));
  flowElement->InsertEndChild (CreateAckPathsElement (flow.GetAckPaths ()));
  flowElement->InsertEndChild (CreateAckShortestPathElement (flow.GetAckShortestPath ()));

  return flowElement;
}

/**
 Generate an XML element housing the Data paths of a given flow.

 @param dataPaths The list of data paths.
 @return The created XMLElement.
 */
XMLElement *
XmlHandler::CreateDataPathsElement (const std::list<Path> &dataPaths)
{
  XMLElement *pathsElement = m_xmlDoc.NewElement ("Paths");
  pathsElement->SetAttribute ("NumPaths", boost::numeric_cast<uint32_t> (dataPaths.size ()));

  for (const auto &path : dataPaths)
    {
      XMLElement *pathElement = m_xmlDoc.NewElement ("Path");
      pathElement->SetAttribute ("Id", path.id);
      pathElement->SetAttribute ("Cost", path.cost);

      for (const auto &linkId : path.GetLinks ())
        {
          XMLElement *linkElement = m_xmlDoc.NewElement ("Link");
          linkElement->SetAttribute ("Id", linkId);
          pathElement->InsertEndChild (linkElement);
        }

      pathsElement->InsertEndChild (pathElement);
    }

  return pathsElement;
}

/**
 Generate an XML element housing the ACK paths of a given flow.

 @param ackPaths The list of ACK paths.
 @return The created XMLElement.
 */
XMLElement *
XmlHandler::CreateAckPathsElement (const std::list<Path> &ackPaths)
{
  XMLElement *pathsElement = m_xmlDoc.NewElement ("AckPaths");
  pathsElement->SetAttribute ("NumPaths", boost::numeric_cast<uint32_t> (ackPaths.size ()));

  for (const auto &path : ackPaths)
    {
      XMLElement *pathElement = m_xmlDoc.NewElement ("Path");
      pathElement->SetAttribute ("Id", path.id);
      pathElement->SetAttribute ("Cost", path.cost);

      for (const auto &linkId : path.GetLinks ())
        {
          XMLElement *linkElement = m_xmlDoc.NewElement ("Link");
          linkElement->SetAttribute ("Id", linkId);
          pathElement->InsertEndChild (linkElement);
        }

      pathsElement->InsertEndChild (pathElement);
    }

  return pathsElement;
}

/**
 Generate an XML element housing the ACK shortest path for a given flow.

 @param ackShortestPath The ACK shortest path.
 @return The created XMLElement.
 */
XMLElement *
XmlHandler::CreateAckShortestPathElement (const Path &ackShortestPath)
{
  XMLElement *ackShortestPathElement = m_xmlDoc.NewElement ("AckShortestPath");

  for (const auto &linkId : ackShortestPath.GetLinks ())
    {
      XMLElement *linkElement = m_xmlDoc.NewElement ("Link");
      linkElement->SetAttribute ("Id", linkId);
      ackShortestPathElement->InsertEndChild (linkElement);
    }

  return ackShortestPathElement;
}

/**
 Add the Network topology element in the result file. This element is used by
 ns3 to build the network topology.

 @param graph The Boost graph.
 */
void
XmlHandler::AddNetworkTopology (const BoostGraph &graph)
{
  XMLElement *netTopElement = m_xmlDoc.NewElement ("NetworkTopology");

  std::list<std::pair<id_t, id_t>> listPairs{FindLinkPairs (graph)};

  for (const auto &listPair : listPairs)
    {
      auto linkAId = listPair.first;
      auto linkBId = listPair.second;

      auto boostLink = graph.GetLink (linkAId);

      XMLElement *linkElement = m_xmlDoc.NewElement ("Link");
      linkElement->SetAttribute ("Delay", graph.GetLinkCost (boostLink));

      if (linkAId == linkBId)
        { // No pair has been found
          linkElement->InsertFirstChild (CreateLinkElement (graph, linkAId));
        }
      else
        { // A pair has been found
          linkElement->InsertFirstChild (CreateLinkElement (graph, linkAId));
          linkElement->InsertFirstChild (CreateLinkElement (graph, linkBId));
        }

      netTopElement->InsertEndChild (linkElement);
    }

  // Add a comment in the XML file that will describe the units being used.
  XMLComment *comment = m_xmlDoc.NewComment ("Delay (ms), Capacity (Mbps), "
                                             "Node Type (T=Terminal, S=Switch)");
  netTopElement->InsertFirstChild (comment);
  m_rootNode->InsertEndChild (netTopElement);
}

/**
 Given a graph, find all the link pairs. A link pair is defined as the two links that
 have opposite source and destination nodes and identical delay values. Note that their
 data rates may be different.

 @param graph The graph.
 @return List of link pairs.
 */
std::list<std::pair<id_t, id_t>>
FindLinkPairs (const BoostGraph &graph)
{
  std::list<std::pair<id_t, id_t>> listPairs;
  std::set<id_t> visitedLinks;

  auto linkIterators = graph.GetLinkIterators ();
  for (auto linkIt = linkIterators.first; linkIt != linkIterators.second; ++linkIt)
    {
      auto linkId = graph.GetLinkId (*linkIt);

      if (visitedLinks.find (linkId) != visitedLinks.end ())
        { // This link has already been saved
          continue;
        }

      auto ret = visitedLinks.emplace (linkId);
      if (!ret.second)
        {
          std::cerr << "Trying to insert a duplicate link when creating the network "
                       "topology element. Link Id "
                    << linkId << std::endl;
        }

      std::list<id_t> oppositeLinks{graph.GetOppositeLink (linkId)};

      // Loop over the list of opposite links to check if there are any available that
      // have not yet been visited.
      for (const auto &oppositeLinkId : oppositeLinks)
        {
          if (visitedLinks.find (oppositeLinkId) == visitedLinks.end ())
            {
              listPairs.emplace_back (linkId, oppositeLinkId);
              visitedLinks.emplace (oppositeLinkId);
              break;
            }
        }
    }

  return listPairs;
}

/**
 Generate an XML element for the given link.

 @param graph The graph.
 @param linkId The link id.
 @return The created XMLElement.
 */
XMLElement *
XmlHandler::CreateLinkElement (const BoostGraph &graph, id_t linkId)
{

  auto link{graph.GetLink (linkId)};

  XMLElement *linkElement = m_xmlDoc.NewElement ("LinkElement");
  linkElement->SetAttribute ("Id", linkId);

  auto srcNode{graph.GetSourceNode (link)};
  auto dstNode{graph.GetDestinationNode (link)};

  linkElement->SetAttribute ("SourceNode", graph.GetNodeId (srcNode));
  std::string srcNodeType{graph.GetNodeType (srcNode)};
  linkElement->SetAttribute ("SourceNodeType", srcNodeType.c_str ());
  linkElement->SetAttribute ("DestinationNode", graph.GetNodeId (dstNode));
  std::string dstNodeType{graph.GetNodeType (dstNode)};
  linkElement->SetAttribute ("DestinationNodeType", dstNodeType.c_str ());
  linkElement->SetAttribute ("Capacity", graph.GetLinkCapacity (link));

  return linkElement;
}

/**
 Save the XML file in the location specified by \p xmlFilePath.

 @param xmlFilePath The full path where to save the result file.
 */
void
XmlHandler::SaveFile (const std::string &xmlFilePath)
{
  if (m_xmlDoc.SaveFile (xmlFilePath.c_str ()) != tinyxml2::XML_SUCCESS)
    {
      throw std::runtime_error ("Could not save XML File in " + xmlFilePath);
    }
}
