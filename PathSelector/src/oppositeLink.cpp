#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "oppositeLink.hpp"

/**
 * Sets the file cursor to the @oppositeLinks section to prepare for parsing
 */
bool
SetFileCursorToOppositeLinksSection (std::ifstream &file)
{
  std::string currentLine;
  std::string oppositeLinksString{"@oppositeLinks"};
  bool oppositeLinksSectionFound{false};

  auto lineNumber{1};
  while (std::getline (file, currentLine))
    {
      lineNumber++;

      // Ignore any comments after the @oppositeLinks section
      if (oppositeLinksSectionFound && currentLine[0] != '#')
        break; // Line found

      // Ignore any trailing white space in the file
      if (oppositeLinksString.compare (0, oppositeLinksString.length (), currentLine, 0,
                                       oppositeLinksString.length ()) == 0)
        {
          oppositeLinksSectionFound = true;
        }
    }

  if (!oppositeLinksSectionFound)
    return false;

  // Move to the beginning of the file
  file.seekg (std::ios::beg);

  // Move one line up to where the opposite links definition starts
  for (auto i = 1; i < (lineNumber - 1); ++i)
    file.ignore (std::numeric_limits<std::streamsize>::max (), '\n'); // Extract and ignore a line

  return true;
}

/**
 * @brief Map the opposite link for a given link
 *
 * This function still requires that the links generated in the map are verified
 * to make sure they exist in the given graph.
 *
 * @param lgfPath The path to the lgf file to parse for building the map
 * @return std::map<id_t, id_t> The populated map
 */
std::map<id_t, id_t>
GenerateOppositeLinkMap (const std::string &lgfPath, const BoostGraph &boostGraph)
{
  try
    {
      std::ifstream lgfFile;
      // Enable exceptions if an error occurs during the file read operation
      lgfFile.exceptions (std::ifstream::badbit);
      lgfFile.open (lgfPath, std::ifstream::in); // Open file as Read Only

      auto oppositeLinkSectionExists = SetFileCursorToOppositeLinksSection (lgfFile);

      if (oppositeLinkSectionExists == false)
        {
          std::cout << "Opposite link section not found. Map will not be built" << std::endl;
          return std::map<id_t, id_t> ();
        }

      LOG_MSG ("Loading the opposite links from " + lgfPath);

      std::map<id_t, id_t> oppositeLinkMap;
      std::string line;
      while (std::getline (lgfFile, line))
        {
          if (!line.empty () && line[0] != '#')
            {
              std::istringstream oppositeLinkSs (line, std::istringstream::in);

              id_t fromLink{0};
              id_t toLink{0};

              oppositeLinkSs >> fromLink >> toLink;

              if (!boostGraph.LinkExists (fromLink) || !boostGraph.LinkExists (toLink))
                {
                  throw std::runtime_error ("The link " + std::to_string (fromLink) + " or " +
                                            std::to_string (toLink) + " does not exist in the map");
                }

              auto [_, insertionSuccessful] = oppositeLinkMap.emplace (fromLink, toLink);
              if (insertionSuccessful == false)
                {
                  throw std::runtime_error ("Trying to insert duplicate link. Link Pair (" +
                                            std::to_string (fromLink) + ", " +
                                            std::to_string (toLink) + ")");
                }
              LOG_MSG ("The opposite link of " << fromLink << " is " << toLink);
            }
        }
      return oppositeLinkMap;
  } catch (const std::ifstream::failure &e)
    {
      std::cerr << "Loading the LGF file failed\n" << lgfPath << "\n" << e.what () << std::endl;
      throw;
  }
}
