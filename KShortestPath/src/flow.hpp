#ifndef flow_parser_hpp
#define flow_parser_hpp

#include <string>
#include <fstream>
#include <map>

#include "definitions.hpp"
#include "path.hpp"

enum class Protocol : char { Tcp = 'T', Ack = 'A', Udp = 'U'};

class Flow
{
public:
  using flowContainer_t = std::map<identifier_t, Flow>;
  using port_t = uint64_t;

  Flow() = delete;
  explicit Flow(const std::string &line);

  // Setters
  void SetSrcPort(port_t srcPort) { this->srcPort = srcPort; }

  // Getters
  identifier_t GetFlowId () const { return id; }
  identifier_t GetSourceId () const { return sourceId; }
  identifier_t GetDestinationId () const { return destinationId; }
  double GetDataRate () const { return dataRate; }
  port_t GetSrcPort () const { return srcPort; }
  port_t GetDstPort () const { return dstPort; }
  uint64_t GetPktSize () const { return packetSize; }
  uint64_t GetNumOfPackets () const { return numOfPackets; }
  Protocol GetProtocol () const { return protocol; }
  uint64_t GetStartTime () const { return startTime; }
  uint64_t GetEndTime () const { return endTime; }
  identifier_t GetTcpFlowId () const { return tcpFlowId; }
  const std::vector<Path>& GetPaths () const { return m_paths; }

  void AddPath (const Path& path) { m_paths.emplace_back (path); }

  Flow& operator= ( const Flow& rhs );

  bool operator < (const Flow& other) const { return id < other.id; }
  friend std::ostream& operator<< (std::ostream& output, const Flow& flow);

private:
  void Parse (const std::string& line);

  // Flow details
  identifier_t id;            // Flow Id
  identifier_t sourceId;      // Source Node Id
  identifier_t destinationId; // Destination Node Id
  port_t srcPort;             // Source Port
  port_t dstPort;             // Destination Port
  double dataRate;            // The flow's data rate including headers
  uint64_t packetSize;        // The flow's packet size including headers
  uint64_t numOfPackets;      // Number of packets the flow is to transmit
  Protocol protocol;          // The protocol the flow can use
  uint64_t startTime;         // Time when to start transmission
  uint64_t endTime;           // Time when to stop transmission
  identifier_t tcpFlowId;     // This is the flow ID of the TCP data stream.
                              // This is only used by ACK flows to determine
                              // to which data flow this ACK flow belongs to.

  std::vector<Path> m_paths;   // List of paths the flow uses
};

/**
 * @brief Load and parse the flows in the given LGF file to the vector m_flows.
 * @param lgfPath The full path to the LGF file.
 * @return Vector of flows
 */
Flow::flowContainer_t ParseFlows (const std::string& lgfPath);

#endif /* flow_parser_hpp */
