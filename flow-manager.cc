#include <iostream>
#include <limits>
#include <sstream>

#include "flow-manager.h"

const std::vector<FlowManager::Flow>*
FlowManager::GetFlows() const
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
#ifdef DEBUG
      std::cout << "Loading flows from: " << lgfPath << std::endl;
      std::cout << "--------------------" << std::endl;
#endif

      // Set the file cursor to the appropriate line
      SetFileCursorToFlows(lgfFile);

      std::string line ("");
      while (std::getline(lgfFile, line))
        {
          ParseFlow(line);
        }

      // Update the source port for TCP flows
      AddTcpSourcePortToTcpFlows ();
    }
  catch (std::ifstream::failure e)
    {
      std::cerr << "Loading the LGF file failed\n" << lgfPath << "\n" << e.what()  << std::endl;
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
  for (int i = 1; i < (lineNumber - 1); i++)
    file.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); // Extracts and ignores a line
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

      // Get Port Number
      flowSs >> parsedValue;
      flow.dstPortNumber = std::stoul (parsedValue);

      // Get Data Rate
      flowSs >> parsedValue;
      flow.dataRate = std::stod (parsedValue);

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
      else if (parsedValue.compare("A") == 0) // TCP Acknowledgement flow
        flow.protocol = FlowManager::Flow::Protocol::Ack;
      else
        throw std::invalid_argument ("Unknown protocol type");

      // Get Start Time
      flowSs >> parsedValue;
      flow.startTime = std::stoul (parsedValue);

      // Get End Time
      flowSs >> parsedValue;
      flow.endTime = std::stoul(parsedValue);

      // Get the TCP Flow Id ONLY if it is an ACK flow
      if (flow.protocol == FlowManager::Flow::Protocol::Ack)
        {
          flowSs >> parsedValue;
          flow.tcpFlowId = std::stoul (parsedValue);
        }

      // Outputting the flow values in debug builds only.
#ifdef DEBUG
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

void
FlowManager::AddTcpSourcePortToTcpFlows ()
{
  try
    {
      // A lambda function that will return a pointer to the tcp flow
      // associated with the ACK flow referenced to by ackFlow
      auto getFlowByID = [this] (Flow& ackFlow) -> Flow*
        {
          for (auto& flow : m_flows)
            {
              // Found the flow. We need to verify that it is correct
              if (flow.id == ackFlow.tcpFlowId)
                {
                  // The flow must be of type TCP and the source and destination
                  // must be swapped.
                  if (flow.protocol == Flow::Protocol::Tcp &&
                      flow.source == ackFlow.destination &&
                      flow.destination == ackFlow.source)
                    return & flow;
                }
            }

          std::string errorMsg ("ERROR: Flow with id " 
                                + std::to_string(ackFlow.tcpFlowId) 
                                + " not found OR Flow does not meet the requirements. "
                                + "The flow must be TCP and the source and destination "
                                + "of the ACK flow must be the reverse of the TCP data "
                                + "flow");
          throw std::invalid_argument (errorMsg);
        };

    for (auto& flow : m_flows) // Loop through all the flows of the Ack type
      {
        if (flow.protocol == Flow::Protocol::Ack)
          {
            Flow* tcpFlow = getFlowByID (flow);
            tcpFlow->srcPortNumber = flow.dstPortNumber;
          }
      }
    }
  catch (std::invalid_argument e)
    {
      std::cerr << e.what() << std::endl;
      throw;
    }
}
