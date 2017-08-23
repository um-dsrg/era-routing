#ifndef FLOW_MANAGER_H
#define FLOW_MANAGER_H

#include <string>
#include <fstream>
#include <stdint.h>
#include <vector>

class FlowManager
{
public:
  /*Structure that defines a flow*/
  struct Flow
  {
    // Default constructor that sets everything to 0.
    Flow () : id (0), source (0), destination(0), dstPortNumber(0), srcPortNumber(0), dataRate(0.0), 
              packetSize(0), numOfPackets(0), protocol(0), startTime(0), endTime(0), tcpFlowId(0)
    {}

    enum Protocol { Tcp = 'T', Ack = 'A', Udp = 'U' };

    uint32_t id;
    uint32_t source;
    uint32_t destination;
    uint32_t dstPortNumber;
    uint32_t srcPortNumber; // So for this is only used for TCP flows
    double dataRate;
    uint32_t packetSize;
    uint32_t numOfPackets;
    char protocol;
    uint32_t startTime;
    uint32_t endTime;
    // This is the flow ID of the TCP data stream. This is only used by ACK
    // flows to determine to which flow this ACK flow belongs to.
    uint32_t tcpFlowId; 

    friend std::ostream& operator<< (std::ostream& output, Flow& flow)
    {
      output << "Id: " << flow.id << " Source: " << flow.source << " Destination: "
             << flow.destination << "\n"
             << "Dst Port Number: " << flow.dstPortNumber;

      if (flow.srcPortNumber != 0)
        output << "Src Port Number: " << flow.srcPortNumber;

      output << " Data Rate: " << flow.dataRate << "Mbps\n"
             << "Packet Size: " << flow.packetSize << "bytes Num Of Packets: "
             << flow.numOfPackets << " Protocol: " << flow.protocol << "\n"
             << "Start Time: " << flow.startTime << "s End Time: " << flow.endTime << "s\n";

      if (flow.protocol == Protocol::Ack)
        output << "TCP Data Flow ID: " << flow.tcpFlowId;

      return output;
    }
  };

  /**
   *  \brief Returns a constant pointer to the flows vector
   *  \return A constant pointer to the flows vector
   */
  const std::vector<Flow>* GetFlows () const;

  /**
   *  \brief Load the Flows from the file given and stores them in the vector flows
   *
   *  Load and parse the flows in the given LGF file. The flows are stored in the vector flows.
   *
   *  \param lgfPath The full path to the LGF file
   *  \return nothing
   */
  void LoadFlowsFromFile (const std::string& lgfPath);

private:
  /**
   *  \brief Sets the cursor to the @flow section
   *
   *  Sets the cursor in the file to the @flow section while
   *  ignoring any comments that begin with the '#' character.
   *
   *  \param file Reference to the file
   */
  void SetFileCursorToFlows (std::ifstream& file);

  /**
   *  \brief Parse the flow into a Flow struct and stores it in the flows vector
   *
   *  Given a line read from the file, creates a Flow structure and stores it in the flows vector
   *
   *  \param line A line read from the LGF file
   *  \return nothing
   */
  void ParseFlow (std::string& line);

  void AddTcpSourcePortToTcpFlows ();

  std::vector<Flow> m_flows;
};
#endif /* FLOW_MANAGER_H */
