#include <iostream>
#include <limits>
#include <sstream>

#include "flow.hpp"

/**
 Path implementation
 */

id_t Path::globalPathId = 0;

/**
 The Path constructor.

 When the flag \p setPathId is set, the path will be assigned an id.
 This functionality is there such that ACK related paths are not assigned
 an id as their path id will be equivalent to the data path id.

 @param setPathId When set the path is assigned an id.
 */
Path::Path(bool setPathId) {
    if (setPathId) {
        id = globalPathId++;
    }
}

/**
 Adds a link to the path's links list

 @param linkId The link id.
 */
void Path::AddLink(id_t linkId) {
    m_links.emplace_back(linkId);
}

/**
 Returns a const reference to the path links.

 @return The path links.
 */
const std::list<id_t>& Path::GetLinks() const {
    return m_links;
}

/**
 Outputs the path object to an output stream.

 @param output The output stream.
 @param path The path instance to be described.
 @return The output stream.
 */
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

/**
 Generate a flow object from a line taken from the LGF file.

 @param line The lgf line describing the flow.
 @param perFlowK Flag that determines whether the K value will be parsed on a
                 per flow basis.
 @param globalK  The global K value that will be used if perFlowK is not set.
 */
Flow::Flow(const std::string &line, bool perFlowK, uint32_t globalK) :
m_ackShortestPath(/* Path id is not required */ false) {
  Parse(line, perFlowK, globalK);
}

/**
 Returns a reference to the list of data paths.

 @return Returns a reference to the list of data paths of the flow.
 */
const std::list<Path>& Flow::GetDataPaths () const {
    return m_dataPaths;
}

/**
 Returns a reference to the list of ack paths.

 @return Returns a reference to the list of acknowledgement paths of the flow.
 */
const std::list<Path>& Flow::GetAckPaths() const {
    return m_ackPaths;
}

/**
 Returns a reference to the path the Acknowledgement flow's shortest path from destination to source.

 @return The acknowledgement path.
 */
const Path& Flow::GetAckShortestPath() const {
    return m_ackShortestPath;
}

/**
 Add a data path to the list of data paths.

 @param path The data path to add in the flow.
 */
void Flow::AddDataPath (const Path& path) {
    m_dataPaths.emplace_back(path);
}

/**
 Add an ACK path to the list of ACK paths.

 @param path The ACK path to add in the flow.
 */
void Flow::AddAckPath(const Path &path) {
    m_ackPaths.emplace_back(path);
}

/**
 Add the shortest ACK path to the flow.

 @param path The shortest ACK path.
 */
void Flow::AddAckShortestPath(const Path &path) {
    m_ackShortestPath = path;
}

/**
 Build a flow object from the flow as described in the LGF file.

 @param line Flow description found in the LGF file.
 @param perFlowK Flag that represents whether or not per flow K is enabled.
 @param globalK The value of the global K value if the \p perFlowK flag
                is not set.
 */
void Flow::Parse(const std::string &line, bool perFlowK, uint32_t globalK) {
    std::istringstream flowSs (line, std::istringstream::in);
    try {
        flowSs >> id >> sourceId >> destinationId >> dataRate >> packetSize
               >> numOfPackets;

        char parsedProtocol;
        flowSs >> parsedProtocol;
        
        if (parsedProtocol == 'T') {
            protocol = Protocol::Tcp;
        } else if (parsedProtocol == 'U') {
            protocol = Protocol::Udp;
        } else {
            throw std::invalid_argument ("Unknown protocol type");
        }
        
        flowSs >> startTime >> endTime;

        if (perFlowK) {
            flowSs >> k;
        } else {
            k = globalK;
        }
    } catch (const std::invalid_argument& ia) {
        std::cerr << "Error when converting flow values OR Invalid Protocol Type\n"
                  << ia.what() << std::endl;
        throw;
    } catch (const std::out_of_range& oor) {
        std::cerr << "Value out of range\n" << oor.what() << std::endl;
        throw;
    }
}

/**
 Returns a string representation of the Flow to the output stream.

 @param output Output stream.
 @param flow The flow to describe.
 @return The output stream.
 */
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
    << " K Value: " << flow.k << "\n"
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

/**
 Parse the LGF file and build the flow objects.

 @param lgfPath The path to the LGF file.
 @param perFlowK The perFlowK flag that is given as command line parameters.
 @param globalK The globalK value if the perFlowK flag is not set.
 @return A container storing all the parsed flows.
 */
Flow::flowContainer_t ParseFlows (const std::string& lgfPath, bool perFlowK, uint32_t globalK) {
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
                Flow flow(line, perFlowK, globalK);
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

/**
 Print all the flow details

 @param flows The flow container.
 */
void PrintFlows (const Flow::flowContainer_t& flows) {
    for (const auto& flowPair : flows) {
        std::cout << flowPair.second;
    }
}
