#include "path_file_parser.h"

using namespace tinyxml2;

void
parseLinks (tinyxml2::XMLNode* rootNode, linkContainer_t& links)
{
  XMLElement* linkElement = rootNode->FirstChildElement("LinkDetails")->FirstChildElement("Link");

  while (linkElement != nullptr)
    {
      id_t linkId;
      linkElement->QueryAttribute("Id", &linkId);
      auto ret = links.insert(std::make_pair(linkId,
                                             std::unique_ptr<Link> (std::make_unique<Link>(linkElement))));

      if (ret.second == false)
        {
          std::stringstream ss;
          ss << "Duplicate link found. Id: " << linkId;
          throw std::runtime_error(ss.str());
        }

      linkElement = linkElement->NextSiblingElement("Link");
    }
}

void
parsePathFile (tinyxml2::XMLNode* rootNode, linkContainer_t& links, pathContainer_t& paths,
               flowContainer_t& flows)
{
  parseLinks(rootNode, links);
  XMLElement* flowElement = rootNode->FirstChildElement("FlowDetails")->FirstChildElement("Flow");

  while (flowElement != nullptr) // Parse flows
    {
      std::string flowProtocol {flowElement->Attribute("Protocol")};
      if (flowProtocol == "T" || flowProtocol == "U") // Only parse TCP/UDP flows
        {
          std::unique_ptr<Flow> flow = std::make_unique<Flow>(flowElement);
          XMLElement* pathElement = flowElement->FirstChildElement("Paths")->FirstChildElement("Path");

          while (pathElement != nullptr) // Parse paths
            {
              std::unique_ptr<Path> path = std::make_unique<Path>(pathElement);
              XMLElement* linkElement = pathElement->FirstChildElement("Link");

              while (linkElement != nullptr) // Parse links
                {
                  id_t linkId {0};
                  linkElement->QueryAttribute("Id", &linkId);
                  std::unique_ptr<Link>& link = links.at(linkId);
                  link->addPath(path.get());
                  path->addLink(link.get());
                  linkElement = linkElement->NextSiblingElement("Link");
                }

              flow->addPath(path.get());
              paths.push_back(std::move(path));
              pathElement = pathElement->NextSiblingElement("Path");
            }
          flows.push_back(std::move(flow));
        }

      flowElement = flowElement->NextSiblingElement("Flow");
    }
}