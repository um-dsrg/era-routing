#include <iostream>
#include <limits>
#include <sstream>

#include "flow-manager.h"

std::vector<FlowManager::Flow>*
FlowManager::GetFlows()
{
  return &m_flows;
}

void
FlowManager::LoadFlowsFromFile(const std::string& lgfPath)
{
  std::ifstream lgfFile;
  // Throw exceptions if an error occurs while reading the file
  lgfFile.exceptions(std::ifstream::badbit);

  try
    {
      lgfFile.open(lgfPath, std::ifstream::in); // Open the file as Read Only
#ifdef MY_DEBUG
      std::cout << "Loading flows from: " << lgfPath << std::endl;
      std::cout << "--------------------" << std::endl;
#endif

      // Set the file cursor to the appropriate line
      SetFileCursorToFlows(lgfFile);

      std::string line ("");
      while (std::getline(lgfFile, line))
        {
          if (line.empty() == false) ParseFlow(line);
        }
    }
  catch (std::ifstream::failure e)
    {
      std::cerr << "Loading the LGF file failed\n" << lgfPath << "\n"
                << e.what() << std::endl;
      throw;
    }
}

void
FlowManager::SetFileCursorToFlows(std::ifstream& file)
{
  std::string line;
  std::string flowSectionString ("@flows");
  bool flowsSectionFound (false);

  int lineNumber (1);
  while (std::getline (file, line))
    {
      lineNumber++;
      // Ignoring any comments after the @flows section
      if (flowsSectionFound && line[0] != '#')
        break; // We found the line where we should start parsing the flows

      // Ignore any trailing white space in the file
      if (flowSectionString.compare(0, flowSectionString.length(), line,
                                    0, flowSectionString.length()) == 0)
        {
          flowsSectionFound = true;
        }
    }

  if (!flowsSectionFound)
    {
      std::cerr << "Flow section not found" << std::endl;
      throw std::invalid_argument ("Flow section not found");
    }

  // Move to the beginning of the file
  file.seekg(std::ios::beg);

  // Move one line up to where the flow definitions begins
  for (int i = 1; i < (lineNumber - 1); i++) {
    // Extract and ignores a line
    file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
  }
}

void
FlowManager::ParseFlow (std::string& line)
{
  FlowManager::Flow flow;
  std::string parsedValue ("");
  std::istringstream flowSs (line, std::istringstream::in);

  try
    {
      // Get Flow ID
      flowSs >> parsedValue;
      flow.id = std::stoul (parsedValue);

      // Get Source Id
      flowSs >> parsedValue;
      flow.source = std::stoul (parsedValue);

      // Get Destination Id
      flowSs >> parsedValue;
      flow.destination = std::stoul (parsedValue);

      // Get Data Rate
      flowSs >> parsedValue;
      flow.requestedDataRate = std::stod (parsedValue);
      flow.allocatedDataRate = flow.requestedDataRate;

      // Get Packet size
      flowSs >> parsedValue;
      flow.packetSize = std::stoul (parsedValue);

      // Get number of packets
      flowSs >> parsedValue;
      flow.numOfPackets = std::stoul (parsedValue);

      // Get Protocol
      flowSs >> parsedValue;
      if (parsedValue.compare("T") == 0)
        flow.protocol = FlowManager::Flow::Protocol::Tcp;
      else if (parsedValue.compare("U") == 0)
        flow.protocol = FlowManager::Flow::Protocol::Udp;
      else
        throw std::invalid_argument ("Unknown protocol type");

      // Get Start Time
      flowSs >> parsedValue;
      flow.startTime = std::stoul (parsedValue);

      // Get End Time
      flowSs >> parsedValue;
      flow.endTime = std::stoul(parsedValue);

      // Outputting the flow values in debug builds only.
#ifdef MY_DEBUG
      std::cout << flow << std::endl;
#endif

      m_flows.push_back(flow);
    }
  catch (std::invalid_argument e)
    {
      std::cerr << "Error when converting flow values OR Invalid Protocol Type\n"
                << e.what() << std::endl;
      throw;
    }
  catch (std::out_of_range e)
    {
      std::cerr << "Value out of range\n" << e.what() << std::endl;
      throw;
    }
}
