#include "path_file_parser.h"

using namespace tinyxml2;

void
parseLinks (tinyxml2::XMLNode *rootNode, linkContainer_t &links)
{
  XMLElement *linkElement = rootNode->FirstChildElement ("LinkDetails")->FirstChildElement ("Link");

  while (linkElement != nullptr)
    {
      id_t linkId;
      linkElement->QueryAttribute ("Id", &linkId);
      auto ret = links.insert (
          std::make_pair (linkId, std::unique_ptr<Link> (std::make_unique<Link> (linkElement))));

      if (ret.second == false)
        {
          std::stringstream ss;
          ss << "Duplicate link found. Id: " << linkId;
          throw std::runtime_error (ss.str ());
        }

      linkElement = linkElement->NextSiblingElement ("Link");
    }
}

void
parsePathFile (tinyxml2::XMLNode *rootNode, linkContainer_t &links, pathContainer_t &paths,
               flowContainer_t &flows)
{
  parseLinks (rootNode, links);
  XMLElement *flowElement = rootNode->FirstChildElement ("FlowDetails")->FirstChildElement ("Flow");

  while (flowElement != nullptr) // Parse flows
    {
      std::string flowProtocol{flowElement->Attribute ("Protocol")};
      if (flowProtocol == "T" || flowProtocol == "U") // Only parse TCP/UDP flows
        {
          /* Used as a temporary map between the path id and it's object
          pointer. This will be used when building the ACK paths */
          std::map<id_t, Path *> tempPathMap;

          std::unique_ptr<Flow> flow = std::make_unique<Flow> (flowElement);

          /* Parsing data paths */
          XMLElement *pathElement =
              flowElement->FirstChildElement ("Paths")->FirstChildElement ("Path");
          while (pathElement != nullptr) // Parse data paths
            {
              std::unique_ptr<Path> path = std::make_unique<Path> (pathElement);
              XMLElement *linkElement = pathElement->FirstChildElement ("Link");

              while (linkElement != nullptr) // Parse links
                {
                  id_t linkId{0};
                  linkElement->QueryAttribute ("Id", &linkId);
                  std::unique_ptr<Link> &link = links.at (linkId);
                  link->addPath (path.get ());
                  path->addLink (link.get ());
                  tempPathMap.emplace (path->getId (), path.get ());

                  linkElement = linkElement->NextSiblingElement ("Link");
                }

              flow->addPath (path.get ());
              paths.push_back (std::move (path));
              pathElement = pathElement->NextSiblingElement ("Path");
            }

          /* Parsing ack paths */
          pathElement = flowElement->FirstChildElement ("AckPaths")->FirstChildElement ("Path");
          while (pathElement != nullptr)
            {
              auto pathId = id_t{0};
              pathElement->QueryAttribute ("Id", &pathId);
              auto path = tempPathMap.at (pathId);

              XMLElement *linkElement = pathElement->FirstChildElement ("Link");
              while (linkElement != nullptr)
                {
                  auto linkId = id_t{0};
                  linkElement->QueryAttribute ("Id", &linkId);
                  auto &link = links.at (linkId);
                  link->addAckPath (path);

                  linkElement = linkElement->NextSiblingElement ("Link");
                }

              pathElement = pathElement->NextSiblingElement ("Path");
            }

          flows.push_back (std::move (flow));
        }

      flowElement = flowElement->NextSiblingElement ("Flow");
    }
}
