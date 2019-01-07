#include <iostream>
#include <tinyxml2.h>
#include <boost/program_options.hpp>

#include "lemon-graph.hpp"

namespace po = boost::program_options;

tinyxml2::XMLElement* CreateLinkElement(tinyxml2::XMLDocument& xml_document, lemon::SmartDigraph::Arc& link, LemonGraph& lemon_graph)
{
    using namespace tinyxml2;
    graph_t& graph = lemon_graph.GetGraph();
    lemon::SmartDigraph::Node sourceNode = graph.source(link);
    lemon::SmartDigraph::Node destinationNode = graph.target(link);
    int sourceNodeId = lemon_graph.GetNodeId(sourceNode);
    int destinationNodeId = lemon_graph.GetNodeId(destinationNode);

    XMLElement* linkElement = xml_document.NewElement("LinkElement");

    linkElement->SetAttribute("Id", graph.id(link));
    linkElement->SetAttribute("SourceNode", sourceNodeId);
    linkElement->SetAttribute("SourceNodeType", std::string(1, lemon_graph.GetNodeType(sourceNode)).c_str());
    linkElement->SetAttribute("DestinationNode", destinationNodeId);
    linkElement->SetAttribute("DestinationNodeType", std::string(1, lemon_graph.GetNodeType(destinationNode)).c_str());
    linkElement->SetAttribute("Capacity", lemon_graph.GetLinkCapacity(link));

    return linkElement;
}

void AddNetworkTopology(LemonGraph& lemon_graph, tinyxml2::XMLDocument& xmlDoc, tinyxml2::XMLNode* rootNode)
{
    using namespace tinyxml2;

    XMLElement* networkTopologyElement = xmlDoc.NewElement("NetworkTopology");
    networkTopologyElement->SetAttribute("NumberOfNodes", lemon::countNodes(lemon_graph.GetGraph()));
    networkTopologyElement->SetAttribute("NumberOfLinks", lemon::countArcs(lemon_graph.GetGraph()));

    std::set<int> visitedLinks;

    // We need to loop through all the links and add their details.
    for (auto link = lemon_graph.GetLinkIt(); link != lemon::INVALID; ++ link)
    {
        int linkId( lemon_graph.GetLinkId(link) );
        int oppositeLinkId{ 0 };

        // This link is already stored in the XML file. Skip it.
        if (visitedLinks.find(linkId) != visitedLinks.end()) continue;

        lemon::SmartDigraph::Arc oppositeLink;

        double currentLinkDelay{ lemon_graph.GetLinkCost(link) };
        bool pairFound(false);

        // Check if link with opposite source and destination exists.
        for (lemon::ConArcIt<lemon::SmartDigraph> oppositeLinkIt(lemon_graph.GetGraph(), lemon_graph.GetGraph().target(link), lemon_graph.GetGraph().source(link));
            oppositeLinkIt != lemon::INVALID; ++oppositeLinkIt)
        {
            oppositeLinkId = lemon_graph.GetLinkId(oppositeLinkIt);
            // This link is already stored in the XML file. Skip it.
            if (visitedLinks.find(oppositeLinkId) != visitedLinks.end()) continue;

            //lemon::SmartDigraph::ArcMap<double> m_linkDelay;
            if (currentLinkDelay == lemon_graph.GetLinkCost(oppositeLinkIt))
            {
                pairFound = true;
                oppositeLink = oppositeLinkIt;
                break;
            }
        }

        if (!pairFound) // If no pair was found issue a warning.
        {
            std::cerr << "Warning: Link " << linkId << " has no opposite link." << std::endl;
            visitedLinks.insert(linkId);

            XMLElement* linkElement = xmlDoc.NewElement("Link");
            linkElement->SetAttribute("Delay", lemon_graph.GetLinkCost(link));
            linkElement->InsertFirstChild(CreateLinkElement(xmlDoc, link, lemon_graph));

            networkTopologyElement->InsertFirstChild(linkElement);
        }
        else // A pair is found.
        {
            // TODO update the below check
            //assert(m_linkDelay[link] == m_linkDelay[oppositeLink]);
            visitedLinks.insert(linkId);
            visitedLinks.insert(oppositeLinkId);

            XMLElement* linkElement = xmlDoc.NewElement("Link");
            linkElement->SetAttribute("Delay", lemon_graph.GetLinkCost(link));
            linkElement->InsertFirstChild(CreateLinkElement(xmlDoc, link, lemon_graph));
            linkElement->InsertFirstChild(CreateLinkElement(xmlDoc, oppositeLink, lemon_graph));

            networkTopologyElement->InsertFirstChild(linkElement);
        }
    }

    // Add a comment in the XML file that will describe the units being used.
    XMLComment* unitsComment =
        xmlDoc.NewComment("Delay (ms), Capacity (Mbps), Node Type (T=Terminal, S=Switch)");
    networkTopologyElement->InsertFirstChild(unitsComment);
    rootNode->InsertEndChild(networkTopologyElement);
}

void AddNodeConfiguration(LemonGraph& lemon_graph, tinyxml2::XMLDocument& xml_document,
    tinyxml2::XMLNode* root_node)
{
    using namespace tinyxml2;
    XMLElement* nodeConfiguration = xml_document.NewElement("NodeConfiguration");

    for (auto node = lemon_graph.GetNodeIt(); node != lemon::INVALID; ++node)
    {
        XMLElement* nodeElement = xml_document.NewElement("Node");
        nodeElement->SetAttribute("Id", static_cast<int>(lemon_graph.GetNodeId(node)));
        nodeElement->SetAttribute("Type", std::string(1, lemon_graph.GetNodeType(node)).c_str());
        nodeElement->SetAttribute("X", 0);
        nodeElement->SetAttribute("Y", 0);

        nodeConfiguration->InsertFirstChild(nodeElement);
    }
    
    root_node->InsertEndChild(nodeConfiguration);
}

int main (int argc, const char *argv[])
{
  std::string lgf_file_path {""};
  std::string output_xml_path{""};

  try {
      po::options_description cmdLineParams("Allowed Options");
      cmdLineParams.add_options()
                    ("help", "Help Message")
                    ("input,i", po::value<std::string>(&lgf_file_path)->required(),
                        "The path to LGF file")
                    ("output,o", po::value<std::string>(&output_xml_path)->required(),
                        "The path where to store the XML file with the topology details");

      po::variables_map vm;
      po::store(po::parse_command_line(argc, argv, cmdLineParams), vm);

      if (vm.count("help")) // Output help message
        {
          std::cout << cmdLineParams << "\n";
          return EXIT_SUCCESS;
        }
      po::notify(vm); // Check if all required parameters are passed

      // Load the LGF file
      LemonGraph lemonGraph;
      lemonGraph.LoadGraphFromFile(lgf_file_path);

      tinyxml2::XMLDocument xml_document;
      tinyxml2::XMLNode* rootElement = xml_document.NewElement("Log");
      tinyxml2::XMLNode* root_node = xml_document.InsertFirstChild(rootElement);
      if (root_node == nullptr)
          throw std::runtime_error("Could not create element node");

      AddNetworkTopology(lemonGraph, xml_document, root_node);
      AddNodeConfiguration(lemonGraph, xml_document, root_node);

      // Save the xml file here!
      if (xml_document.SaveFile(output_xml_path.c_str()) != tinyxml2::XML_SUCCESS)
          throw std::runtime_error("Could not save XML File");

  } catch (std::exception& e)
  {
      std::cerr << "Error: " << e.what() << "\n";
      return EXIT_FAILURE;
  } catch (...)
  {
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
