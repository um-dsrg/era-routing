#include <iostream>
#include <limits>
#include <sstream>

#include "flow.hpp"

Flow::Flow(const std::string &line) : srcPort (0)
{
  Parse(line);
}

Flow&
Flow::operator=(const Flow &rhs)
{
  if (&rhs == this) return *this; // Check for self-assignment

  id = rhs.id;
  sourceId = rhs.sourceId;
  destinationId = rhs.destinationId;
  dstPort = rhs.dstPort;
  dataRate = rhs.dataRate;
  packetSize = rhs.packetSize;
  numOfPackets = rhs.numOfPackets;
  protocol = rhs.protocol;
  startTime = rhs.startTime;
  endTime = rhs.endTime;
  tcpFlowId = rhs.tcpFlowId;

  return *this;
}

std::ostream&
operator<< (std::ostream& output, const Flow& flow)
{
  output << "Id: " << flow.id
         << " Source: " <<  flow.GetSourceId()
         << " Destination: " << flow.GetDestinationId() << "\n"
         << "Dst Port Number: " << flow.dstPort;

  output << " Data Rate: " << flow.dataRate << "Mbps\n"
         << "Packet Size: " << flow.packetSize << "bytes Num Of Packets: "
         << flow.numOfPackets << " Protocol: "
         << static_cast<std::underlying_type<Protocol>::type>(flow.protocol)
         << "\n" << "Start Time: " << flow.startTime
         << "s End Time: " << flow.endTime << "s\n";

  if (flow.protocol == Protocol::Ack)
    output << "TCP Data Flow ID: " << flow.tcpFlowId;

  return output;
}

void
Flow::Parse(const std::string &line)
{
  std::istringstream flowSs (line, std::istringstream::in);

  try
    {
      flowSs >> id >> sourceId >> destinationId >> dstPort >> dataRate
             >> packetSize >> numOfPackets;

      char parsedProtocol;
      flowSs >> parsedProtocol;

      if (parsedProtocol == 'T') protocol = Protocol::Tcp;
      else if (parsedProtocol == 'U') protocol = Protocol::Udp;
      else if (parsedProtocol == 'A') protocol = Protocol::Ack;
      else throw std::invalid_argument ("Unknown protocol type");

      flowSs >> startTime >> endTime;

      // Retrieve the TCP Flow Id ONLY if it is an ACK flow
      if (protocol == Protocol::Ack)
        flowSs >> tcpFlowId;
    }
  catch (std::invalid_argument& e)
    {
      std::cerr << "Error when converting flow values OR Invalid Protocol Type\n"
      << e.what() << std::endl;
      throw;
    }
  catch (std::out_of_range& e)
    {
      std::cerr << "Value out of range\n" << e.what() << std::endl;
      throw;
    }
}

void SetFileCursorToFlowsSection (std::ifstream& file);
void UpdateTcpFlowsWithSrcPort (Flow::flowContainer_t& flows);

Flow::flowContainer_t
ParseFlows (const std::string& lgfPath)
{
  std::ifstream lgfFile;
  // Enable exceptions if error occurs during file read
  lgfFile.exceptions(std::ifstream::badbit);

  Flow::flowContainer_t flows;

  try
    {
      LOG_MSG("Loading flows from " + lgfPath);
      lgfFile.open(lgfPath, std::ifstream::in); // Open file as Read Only

      // Set the file cursor to the flows section
      SetFileCursorToFlowsSection(lgfFile);

      std::string line;
      while (std::getline(lgfFile, line))
        {
          if (!line.empty())
            {
              Flow flow(line);
              auto ret = flows.emplace (std::make_pair (flow.GetFlowId (),
                                                        flow));
              if (!ret.second)
                  std::cerr << "ERROR: Trying to insert a duplicate flow. "
                               "Flow Id: " << flow.GetFlowId ();
            }
        }
    }
  catch (std::ifstream::failure& e)
    {
      std::cerr << "Loading the LGF file failed\n" << lgfPath << "\n"
                << e.what()  << std::endl;
      throw;
    }

  UpdateTcpFlowsWithSrcPort (flows);

  return flows;
}

/**
 Sets the cursor in the file passed as parameter to this
 function to the @flow section while ignoring any comments
 that begin with the '#' character.

 @param file Reference to the lgf file.
 */
void
SetFileCursorToFlowsSection(std::ifstream& file)
{
  std::string line;
  std::string flowSectionString ("@flows");
  bool flowsSectionFound (false);

  int lineNumber (1);
  while (std::getline (file, line))
    {
      lineNumber++;

      // Ignore any comments after the @flows section
      if (flowsSectionFound && line[0] != '#')
        break; // Line found

      // Ignore any trailing white space in the file
      if (flowSectionString.compare(0, flowSectionString.length(), line,
                                    0, flowSectionString.length()) == 0)
        {
          flowsSectionFound = true;
        }
    }

  if (!flowsSectionFound)
    throw std::invalid_argument ("Flow section not found");

  // Move to the beginning of the file
  file.seekg(std::ios::beg);

  // Move one line up to where the flow definitions begins
  for (int i = 1; i < (lineNumber - 1); i++)
    {
      // Extract and ignore a line
      file.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
    }
}

/**
 * @brief Update the source port of the TCP data flows
 *
 * Update the source port of the TCP data flows to be equal to its respective
 * ACK flow's destination port.
 *
 * @param flows
 */
void UpdateTcpFlowsWithSrcPort (Flow::flowContainer_t& flows)
{
  for (auto& flowEntry : flows)
    {
      Flow& flow = flowEntry.second;

      if (flow.GetProtocol () == Protocol::Ack)
        {
          identifier_t tcpDataFlowId = flow.GetTcpFlowId();
          try
            {
              Flow& tcpDataFlow = flows.at(tcpDataFlowId);
              tcpDataFlow.SetSrcPort (flow.GetDstPort ());
            }
          catch (std::out_of_range& e)
            {
              std::cerr << "Flow ID: " << tcpDataFlowId << " not found";
              throw;
            }
        }
    }
}
