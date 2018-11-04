#!/usr/bin/env python3
"""
Generate a random flow set given a base LGF file.

The flows' source and destination nodes are based on the node's output and
input capacity respectively. The more capacity the node has, the more likely
it is to be chosen.
"""
import argparse
import numpy as np


def convert_tabs_to_spaces(string, tab_stop=8):
    """Converts tabs to spaces"""
    result = str()

    for character in string:
        if character == "\t":
            current_position = len(result)
            next_position = current_position + tab_stop - \
                (current_position % tab_stop)
            spaces_to_insert = next_position - current_position
            for _ in range(0, spaces_to_insert):
                result += " "
        else:
            result += character

    return result


class Node:
    """Class representing a node"""
    max_node_id = 0  # The largest value of the node id

    def __init__(self, lgf_line):
        """
        Initialise the node from a line taken from the LGF file under the
        @nodes section

        Keyword Arguments:
        lgf_line -- A line from the LGF file used to initialise this node
        """
        self.outgoing_capacity = 0.0
        self.incoming_capacity = 0.0
        self.node_id = self.__get_node_id_from_line(lgf_line)
        Node.max_node_id = max(Node.max_node_id, self.node_id)

    @staticmethod
    def __get_node_id_from_line(lgf_line):
        """
        Parse an LGF line and return the node id
        Keyword Arguments:
        lgf_line -- A line from the LGF file that describes a node
        """
        line_split = lgf_line.split()

        # Check that node is a switch
        if line_split[2] != 'S':
            raise RuntimeError('A non-switch type node found')

        return int(line_split[0])

    def __repr__(self):
        return ('Max Node Id: {}\nNode Id: {} Outgoing capacity: {} '
                'Incoming capacity: {}'
                .format(Node.max_node_id, self.node_id, self.outgoing_capacity,
                        self.incoming_capacity))


class Flow:
    """Class representing a flow"""
    # Video resolution probabilities. Set by cmd line arguments
    vid_res_prob = None

    # Default Values
    flow_id = 0         # Flow Id counter
    port_number = 1399  # Port number counter
    packet_size = 590   # Packet size
    num_packets = 100   # Num Packets

    def __init__(self, src_node, dst_node, protocol, tcp_flow=None):
        """
        Note: At first the source and destination nodes will refer to switch
        nodes and will later be updated to terminal link ids.

        Keyword Arguments:
        src_node -- The flow's source node
        dst_node -- The flow's destination node
        protocol -- The flow's protocol. TCP/UDP/ACK
        tcp_flow -- When protocol is ACK, tcp_flow is the TCP flow
        """
        self.flow_id = self.__get_flow_id()
        self.src_node = src_node
        self.dst_node = dst_node
        self.port_number = self.__get_port_number()
        self.packet_size = Flow.packet_size
        self.num_packets = Flow.num_packets
        self.protocol = protocol
        self.start_time = 0
        self.end_time = 0

        if self.protocol == 'A':  # ACK Flow
            self.tcp_flow_id = tcp_flow.flow_id
            self.data_rate = 0.1
        else:  # TCP/UDP Flow
            self.data_rate = self.generate_random_data_rate()
            self.tcp_flow_id = '-'

    def __str__(self):
        flow_str = ('{}\t{}\t{}\t{}\t{:.2f}\t{}\t{}\t{}\t{}\t{}\t{}\n'
                    .format(self.flow_id, self.src_node, self.dst_node,
                            self.port_number, self.data_rate, self.packet_size,
                            self.num_packets, self.protocol, self.start_time,
                            self.end_time, self.tcp_flow_id))
        return convert_tabs_to_spaces(flow_str)

    @staticmethod
    def __get_port_number():
        """Returns the port number"""
        Flow.port_number += 1
        return Flow.port_number

    @staticmethod
    def __get_flow_id():
        """Returns the current flow id"""
        Flow.flow_id += 1
        return Flow.flow_id

    @staticmethod
    def generate_random_data_rate():
        """
        Randomly generates the flow's data rate using the set distributions
        """
        available_vid_res = ['sd', 'hd', 'uhd']
        vid_res = np.random.choice(available_vid_res, 1, p=Flow.vid_res_prob)

        if vid_res == 'sd':
            mean = 3.0
            std_deviation = 0.1
        elif vid_res == 'hd':
            mean = 5.0
            std_deviation = 0.25
        elif vid_res == 'uhd':
            mean = 25.0
            std_deviation = 2.5
        else:
            raise RuntimeError('The selected video type does not exist')

        return np.random.normal(mean, std_deviation, 1)[0]


def parse_link_from_line_and_scale(lgf_line, nodes, scaling_factor):
    """
    Parses the link LGF line to update the node's total outgoing and incoming
    capacity. The link capacity is also scaled based on the scaling factor
    passed as parameter.

    Keyword Arguments:
    lgf_line       -- A line from the LGF file that describes a link
    nodes          -- The dictionary storing information about the nodes
    scaling_factor -- The value to multiply the link capacity with. Used to
                      scale the network

    Returns:
    A tuple containing the link id, and an LGF string representation of the
    scaled version of the link
    """
    line_split = lgf_line.split()
    src_node = int(line_split[0])
    dst_node = int(line_split[1])
    link_capacity = float(line_split[3]) * scaling_factor

    nodes[src_node].outgoing_capacity += link_capacity
    nodes[dst_node].incoming_capacity += link_capacity

    link_id = int(line_split[2])
    link_delay = line_split[4]
    link_str = (str(src_node) + '\t' + str(dst_node) + '\t' + str(link_id) +
                '\t' + str(link_capacity) + '\t\t' + link_delay + '\n')

    return link_id, link_str


def parse_lgf_file(base_lgf_file, scaling_factor):
    """
    Parse the LGF file to build the nodes information

    Keyword Arguments:
    base_lgf_file  --
    scaling_factor --

    Returns:
    A tuple of:
      - Dictionary containing the information of each node
      - A list of strings that represent the LGF file with scaled links
      - The largest link id value met during parsing
    """
    nodes = dict()  # Key -> node_id Value -> Node object
    max_link_id = 0  # The largest value of the link id met
    updated_lgf_file = list()  # List of str representing the updated LGF file

    with open(base_lgf_file) as lgf_file:
        parse_links = False
        for line in lgf_file:
            # Ignore lines that do not start with a number
            if line[0].isdigit():
                if parse_links:  # Parse the links
                    link_id, line = \
                        parse_link_from_line_and_scale(line,
                                                       nodes,
                                                       scaling_factor)
                    max_link_id = max(max_link_id, link_id)
                else:  # Parse nodes
                    node = Node(line)
                    nodes[node.node_id] = node

            if line == '@arcs\n':  # Link section found in the LGF file
                parse_links = True

            updated_lgf_file.append(convert_tabs_to_spaces(line))

    return nodes, updated_lgf_file, max_link_id


def generate_random_flow_set(nodes, flow_protocol, num_flows_to_generate):
    """
    Generate a random flow set with the source and destination nodes picked
    randomly based on the node's outgoing and incoming capacity respectively.

    Keyword Arguments:
    nodes                 -- Dictionary containing the information of each node
    flow_protocol         -- The transport protocol the generated flows will
                             use
    num_flows_to_generate -- The number of flows to generate

    Returns:
    A dictionary with the Flow Id as key and the respective Flow object as
    value.
    """
    node_ids = [node_id for node_id in sorted(nodes.keys())]

    node_out_cap = [nodes[node_id].outgoing_capacity for node_id in node_ids]
    tot_out_cap = sum(node_out_cap)
    node_out_cap_fraction = [cap/tot_out_cap for cap in node_out_cap]

    node_in_cap = [nodes[node_id].incoming_capacity for node_id in node_ids]
    tot_in_cap = sum(node_in_cap)
    node_in_cap_fraction = [cap/tot_in_cap for cap in node_in_cap]

    flows = dict()
    for _ in range(0, num_flows_to_generate):
        src_node = dst_node = 0
        while src_node == dst_node:
            src_node = np.random.choice(node_ids, 1,
                                        p=node_out_cap_fraction)[0]
            dst_node = np.random.choice(node_ids, 1,
                                        p=node_in_cap_fraction)[0]

        data_flow = Flow(src_node, dst_node, flow_protocol)
        flows[data_flow.flow_id] = data_flow

        if flow_protocol == 'T':  # If TCP flow, create ACK flow
            ack_flow = Flow(dst_node, src_node, 'A', data_flow)
            flows[ack_flow.flow_id] = ack_flow

    return flows


def add_terminals_to_lgf_file(flows, updated_lgf_file):
    """
    Adds the required terminals to the LGF file.

    Keyword Arguments:
    flows            -- A dictionary with the Flow Id as key and the
                        respective Flow object as value
    updated_lgf_file -- List of strings representing the LGF file

    Returns:
    A dictionary mapping the terminal node connected to a particular switch
    Key: Switch ID
    Value: Terminal ID
    """
    # Add the terminals to the LGF file
    nodes_used_by_flows = set()  # Store the nodes/switches used by the flows

    for flow in flows.values():
        nodes_used_by_flows.add(flow.src_node)
        nodes_used_by_flows.add(flow.dst_node)

    terminal_ins_loc = updated_lgf_file.index('@arcs\n')
    updated_lgf_file.insert(terminal_ins_loc, '# User Generated nodes\n')
    terminal_ins_loc += 1

    # Key -> Node Id representing the switch Value -> Node Id
    switch_to_terminal = dict()
    terminal_id = Node.max_node_id + 1

    for switch_id in nodes_used_by_flows:
        switch_to_terminal[switch_id] = terminal_id
        terminal_str = '{}\t(0,0)\t\tT\n'.format(terminal_id)
        updated_lgf_file.insert(terminal_ins_loc,
                                convert_tabs_to_spaces(terminal_str))
        terminal_id += 1
        terminal_ins_loc += 1

    return switch_to_terminal


def add_links_to_lgf_file(switch_to_terminal, max_link_id, terminal_switch_dr,
                          updated_lgf_file):
    """
    Add the links required to the LGF file

    Keyword Arguments:
    switch_to_terminal -- A dictionary mapping the terminal node connected to
                          a particular switch
    max_link_id        -- The largest link id. Used to assign an ID to the
                          links that has not been used yet.
    terminal_switch_dr -- The data rate of the links joining the terminals
                          with the switches
    updated_lgf_file   -- List of strings representing the LGF file
    """
    link_ins_loc = updated_lgf_file.index('@flows\n')
    updated_lgf_file.insert(link_ins_loc, '# User Generated links\n')
    link_ins_loc += 1

    link_id = max_link_id + 1
    for switch_id, terminal_id in switch_to_terminal.items():
        sw_to_t = (str(switch_id) + '\t' + str(terminal_id) + '\t' +
                   str(link_id) + '\t' + str(terminal_switch_dr) + '\t\t' +
                   str(1.0) + '\n')
        updated_lgf_file.insert(link_ins_loc, convert_tabs_to_spaces(sw_to_t))
        link_id += 1
        link_ins_loc += 1

        t_to_sw = (str(terminal_id) + '\t' + str(switch_id) + '\t' +
                   str(link_id) + '\t' + str(terminal_switch_dr) + '\t\t' +
                   str(1.0) + '\n')
        updated_lgf_file.insert(link_ins_loc, convert_tabs_to_spaces(t_to_sw))
        link_id += 1
        link_ins_loc += 1


def add_flows_to_lgf_file(flows, switch_to_terminal, updated_lgf_file):
    """
    Add the flows to the LGF file and update the flows' source and destination
    nodes to use terminal node ids instead of switch ids.

    Keyword Arguments:
    flows              -- A dictionary with the Flow Id as key and the
                          respective Flow object as value
    switch_to_terminal -- A dictionary mapping the terminal node connected to
                          a particular switch
    updated_lgf_file   -- List of strings representing the LGF file
    """
    updated_lgf_file.append('# Flow ID / Source Node / Destination Node / '
                            'Port Number / DataRate Incl. Headers (Mbps) / '
                            'Packet Size  Incl. Headers(bytes) / '
                            'No. of Packets / Protocol (TCP=T/UDP=U/ACK=A) / '
                            'Start Time (Seconds) / End Time (Seconds) / '
                            'TCP Flow Id\n')
    updated_lgf_file.append('# TCP Flow Id is ONLY used by Acknowledgement '
                            'type flows to identify the TCP data flow they '
                            'belong to.\n')
    updated_lgf_file.append('# A is the Acknowledgement flow that TCP will '
                            'use to send back acknowledgements.\n')
    updated_lgf_file.append('#ID     Source  Dest    Port    DR      PS      '
                            'NP      Prtcl   Start   End     TCP Flow Id\n')

    # Set the Flow source and destination nodes to be equal to the terminal
    # nodes instead of the switches
    for flow in flows.values():
        flow.src_node = switch_to_terminal[flow.src_node]
        flow.dst_node = switch_to_terminal[flow.dst_node]
        updated_lgf_file.append(str(flow))


def save_lgf_file(updated_lgf_file, file_path):
    """
    Save the updated LGF file
    Keyword Arguments:
    updated_lgf_file -- List of strings representing the LGF file
    file_path        -- The path where to save the new file
    """
    with open(file_path, 'w') as new_file:
        new_file.writelines(updated_lgf_file)


def main():
    """The main function"""
    parser = argparse.ArgumentParser()
    parser.add_argument('--base_lgf_file', type=str, required=True,
                        help='The full path to the base LGF file')
    parser.add_argument('--save_lgf_file', type=str, required=True,
                        help='The full path to the new generated LGF file')
    parser.add_argument('--num_flows', type=int, required=True,
                        help='The number of flows to generate')
    parser.add_argument('--scaling_factor', type=float, required=True,
                        help='The scaling factor to multiply the links with')
    parser.add_argument('--flow_protocol', type=str, required=True,
                        help='The flows\' protocol. T=TCP U=UDP')
    parser.add_argument('--terminal_to_switch_dr', type=float, required=False,
                        default=10000,
                        help='The terminal <-> switch link data rate. Default '
                             'value of 10,000')
    parser.add_argument('--vid_res_prob', nargs='*', type=float,
                        required=False, default=[1.0, 0.5, 0.5],
                        help='The probabilities of selecting a video '
                             'resolution. The current available resolutions '
                             'are: SD, HD, UHD. Default probabilities are: '
                             'SD: 0 HD: 50% UHD: 50%')

    args = parser.parse_args()

    base_lgf_file = args.base_lgf_file
    lgf_file_path = args.save_lgf_file
    scaling_factor = args.scaling_factor
    num_flows = args.num_flows
    flow_protocol = args.flow_protocol
    terminal_to_switch_dr = args.terminal_to_switch_dr
    Flow.vid_res_prob = args.vid_res_prob  # Set the video resolution

    nodes, updated_lgf_file, max_link_id = parse_lgf_file(base_lgf_file,
                                                          scaling_factor)

    flows = generate_random_flow_set(nodes, flow_protocol, num_flows)

    switch_to_terminal = add_terminals_to_lgf_file(flows, updated_lgf_file)

    add_links_to_lgf_file(switch_to_terminal, max_link_id,
                          terminal_to_switch_dr, updated_lgf_file)

    add_flows_to_lgf_file(flows, switch_to_terminal, updated_lgf_file)

    save_lgf_file(updated_lgf_file, lgf_file_path)


if __name__ == '__main__':
    main()
