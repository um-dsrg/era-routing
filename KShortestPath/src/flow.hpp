#ifndef flow_parser_hpp
#define flow_parser_hpp

#include <map>
#include <list>
#include <string>
#include <fstream>

#include "definitions.hpp"

enum class Protocol : char {Tcp='T', Udp='U', Undefined='X'};

struct Path {
    static id_t globalPathId;
    
    id_t id {0};
    linkCost_t cost {0};
    
    Path(bool setPathId);
    void AddLink (id_t linkId);
    const std::list<id_t>& GetLinks() const;
    bool operator< (const Path& other) const;
    
    friend std::ostream& operator<< (std::ostream& output, const Path& path);

private:
    std::list<id_t> m_links; /**< List of link ids the path goes through. */
};

struct Flow {
    using flowContainer_t = std::map<id_t, Flow>;
    using dataRate_t = double;
    using port_t = uint16_t;
    
    id_t id{0};                             // Flow Id
    id_t sourceId{0};                       // Source Node Id
    id_t destinationId{0};                  // Destination Node Id
    dataRate_t dataRate{0.0};               // The flow's data rate including headers
    uint64_t packetSize{0};                 // The flow's packet size including headers
    uint64_t numOfPackets{0};               // Number of packets the flow is to transmit
    Protocol protocol{Protocol::Undefined}; // The protocol the flow can use
    uint64_t startTime{0};                  // Time when to start transmission
    uint64_t endTime{0};                    // Time when to stop transmission
    uint32_t k{0};                          // The flow specific k value

    Flow() = delete;
    explicit Flow(const std::string &line, bool perFlowK, uint32_t globalK);

    const std::list<Path>& GetDataPaths() const;
    const std::list<Path>& GetAckPaths() const;
    void AddDataPath(const Path& path);
    void AddAckPath(const Path& path);

    bool operator< (const Flow& other) const;
    friend std::ostream& operator<< (std::ostream& output, const Flow& flow);

private:
    void Parse (const std::string& line, bool perFlowK, uint32_t globalK);
    std::list<Path> m_dataPaths; /**< List of paths the data flow will use */
    std::list<Path> m_ackPaths; /**< List of paths the ack flow will use */
};

/**
 * @brief Load and parse the flows in the given LGF file to the vector m_flows.
 * @param lgfPath The full path to the LGF file.
 * @return Vector of flows
 */
Flow::flowContainer_t ParseFlows (const std::string& lgfPath, bool perFlowK, uint32_t globalK);

void PrintFlows (const Flow::flowContainer_t& flows);
    
#endif /* flow_parser_hpp */
