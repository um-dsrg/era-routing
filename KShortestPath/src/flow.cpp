#include <iostream>
#include <limits>
#include <sstream>

#include "flow.hpp"

/**
 Path implementation
 */

id_t Path::globalPathId = 0;

Path::Path(bool setPathId) {
    if (setPathId) {
        id = globalPathId++;
    }
}

void Path::AddLink(id_t linkId) {
    m_links.emplace_back(linkId);
}

const std::list<id_t>& Path::GetLinks() const {
    return m_links;
}

/**
 * @brief Overloads the Path::operator < to compare to paths. Used for sorting
 */
bool Path::operator<(const Path& other) const {
    return cost < other.cost;
}

std::ostream& operator<< (std::ostream& output, const Path& path) {
    output << "  Path ID: " << path.id << "\n"
           << "  Path Cost: " << path.cost << "\n"
           << "  Links: ";
    
    for (const auto& link : path.m_links) {
        output << link << " ";
    }
    
    return output;
}

/**
 Flow implementation
 */

Flow::Flow(const std::string &line) {
  Parse(line);
}

const std::list<Path>& Flow::GetDataPaths () const {
    return m_dataPaths;
}

const std::list<Path>& Flow::GetAckPaths() const {
    return m_ackPaths;
}

void Flow::AddDataPath (const Path& path) {
    m_dataPaths.emplace_back(path);
}

void Flow::AddAckPath(const Path &path) {
    m_ackPaths.emplace_back(path);
}

bool Flow::operator<(const Flow &other) const {
    return id < other.id;
}

void Flow::Parse(const std::string &line) {
    
    std::istringstream flowSs (line, std::istringstream::in);
    try {
        auto tempDstPort{0}; // Port number is no longer needed; thus, it is ignored
        flowSs >> id >> sourceId >> destinationId >> tempDstPort >> dataRate
               >> packetSize >> numOfPackets;
        
        char parsedProtocol;
        flowSs >> parsedProtocol;
        
        if (parsedProtocol == 'T') {
            protocol = Protocol::Tcp;
        } else if (parsedProtocol == 'U') {
            protocol = Protocol::Udp;
        } else if (parsedProtocol == 'A') {
            protocol = Protocol::Ack;
        } else {
            throw std::invalid_argument ("Unknown protocol type");
        }
        
        flowSs >> startTime >> endTime;
    } catch (const std::invalid_argument& ia) {
        std::cerr << "Error when converting flow values OR Invalid Protocol Type\n"
                  << ia.what() << std::endl;
        throw;
    } catch (const std::out_of_range& oor) {
        std::cerr << "Value out of range\n" << oor.what() << std::endl;
        throw;
    }
}

std::ostream& operator<< (std::ostream& output, const Flow& flow) {
    output << "----------\n"
    << "Id: " << flow.id << "\n"
    << " Source: " <<  flow.sourceId << "\n"
    << " Destination: " << flow.destinationId << "\n"
    << " Data Rate: " << flow.dataRate << "Mbps\n"
    << " Packet Size: " << flow.packetSize << "bytes\n"
    << " Number of packets: " << flow.numOfPackets << "\n"
    << " Protocol: " << static_cast<char>(flow.protocol) << "\n"
    << " Start Time: " << flow.startTime << "s\n"
    << " End Time: " << flow.endTime << "s\n"
    << "----------\n";

    if (!flow.m_dataPaths.empty()) {
        output << "Data Paths\n"
               << "----------\n";
        for (const auto& path : flow.m_dataPaths) {
            output << path << "\n----------\n";
        }
    }

    if (!flow.m_ackPaths.empty()) {
        output << "Ack Paths\n"
               << "----------\n";
        for (const auto& path : flow.m_ackPaths) {
            output << path << "\n----------\n";
        }
    }

    return output;
}

/**
 Sets the cursor in the file passed as parameter to the '@flow' section
 while ignoring any comments that begin with the '#' character.

 @param file Reference to the lgf file stream.
 */
void SetFileCursorToFlowsSection(std::ifstream& file) {
    std::string line;
    std::string flowSectionString ("@flows");
    bool flowsSectionFound (false);
    
    int lineNumber (1);
    while (std::getline (file, line)) {
        lineNumber++;
        
        // Ignore any comments after the @flows section
        if (flowsSectionFound && line[0] != '#') {
            break; // Line found
        }
        
        // Ignore any trailing white space in the file
        if (flowSectionString.compare(0, flowSectionString.length(), line,
                                      0, flowSectionString.length()) == 0) {
            flowsSectionFound = true;
        }
    }
    
    if (!flowsSectionFound) {
        throw std::invalid_argument ("Flow section not found");
    }
    
    // Move to the beginning of the file
    file.seekg(std::ios::beg);
    
    // Move one line up to where the flow definitions begins
    for (int i = 1; i < (lineNumber - 1); i++) {
        file.ignore(std::numeric_limits<std::streamsize>::max(),'\n'); // Extract and ignore a line
    }
}


Flow::flowContainer_t ParseFlows (const std::string& lgfPath) {
    Flow::flowContainer_t flows;
    
    try {
        std::ifstream lgfFile;
        lgfFile.exceptions(std::ifstream::badbit); // Enable exceptions if error occurs during file read
        lgfFile.open(lgfPath, std::ifstream::in);  // Open file as Read Only
        SetFileCursorToFlowsSection(lgfFile);      // Set the file cursor to the flows section
        
        LOG_MSG("File read successfully. Loading flows from " + lgfPath);
        
        std::string line;
        while (std::getline(lgfFile, line)) {
            if (!line.empty()) {
                Flow flow(line);
                
                if (flow.protocol == Protocol::Ack) { // Ignore Acknowledgement flows
                    std::cout << "Warning: Acknowledgement flows in the LGF will be ignored and will "
                                 "be setup automatically" << std::endl;
                    continue;
                }
                
                auto ret = flows.emplace(flow.id, flow);
                if (!ret.second) {
                    std::cerr << "ERROR: Trying to insert a duplicate flow. Flow Id: " << flow.id;
                }
                LOG_MSG(flow);
            }
        }
    } catch (const std::ifstream::failure& e) {
        std::cerr << "Loading the LGF file failed\n" << lgfPath << "\n"
        << e.what()  << std::endl;
        throw;
    }
    
    return flows;
}

void PrintFlows (const Flow::flowContainer_t& flows) {
    for (const auto& flowPair : flows) {
        std::cout << flowPair.second;
    }
}
