class _Path:
    """Class representing a Path.

    Attributes
        id:    The path id.
        cost:  The paths's cost.
        links: List of links the path uses.
    """

    def __init__(self, path_element):
        self.id = int(path_element.get('Id'))
        self.cost = float(path_element.get('Cost'))
        self.links = [int(link_element.get('Id')) for link_element
                      in path_element.findall('Link')]

    def __repr__(self):
        return 'Path(' + self.__str__() + ')'

    def __str__(self):
        return ('Path ID: {} Path Cost: {} Links used: {}'.format(self.id,
                                                                  self.cost,
                                                                  self.links))

class Flow:
    """Class representing a flow.

    Attributes
        paths: A dictionary that will store the flow's paths where Key is the
               Path Id and Value is the Path object.
    """

    def __init__(self, flow_element):
        self.paths = dict()  # type: Dict[int, _Path]
        self._generate_flow_from_element(flow_element)

    def get_paths(self):
        """Return a list of the paths the flow uses."""
        return list(self.paths.values())

    def get_num_paths(self):
        """Return the number of paths the flow uses."""
        return len(self.paths)

    def get_path_ids(self):
        """Return a list of path ids the flow uses."""
        return self.paths.keys()

    def get_path_costs(self):
        """Return a list of path costs the flow uses."""
        return [path.cost for path in self.paths.values()]

    def _generate_flow_from_element(self, flow_element):
        """Parse the XML <Flow> element and build the Flow object.

        :param flow_element: The Flow XML element.
        """
        self.id = int(flow_element.get('Id'))
        self.src_node = int(flow_element.get('SourceNode'))
        self.dst_node = int(flow_element.get('DestinationNode'))
        self.requested_rate = float(flow_element.get('DataRate'))
        self.pkt_size = int(flow_element.get('PacketSize'))
        self.num_packets = int(flow_element.get('NumOfPackets'))
        self.protocol = str(flow_element.get('Protocol'))
        self.start_time = int(flow_element.get('StartTime'))
        self.end_time = int(flow_element.get('EndTime'))

        if self.protocol == 'T':
            self.src_port = int(flow_element.get('SrcPortNumber'))
            self.dst_port = int(flow_element.get('DstPortNumber'))
        else:
            self.dst_port = int(flow_element.get('PortNumber'))

        if self.protocol == 'A':
            self.tcp_flow_id = int(flow_element.get('TcpFlowId'))

        # Create paths
        for path_element in flow_element.findall('Paths/Path'):
            path = _Path(path_element)

            if path.id in self.paths:
                raise AssertionError('Path {} is duplicate in flow {}'
                                     .format(self.id, path.id))

            self.paths[path.id] = path

    def __repr__(self):
        return 'Flow(' + self.__str__() + ')'

    def __str__(self):
        return ('Flow Id: {} Requested Data Rate: {} Number of Paths: {}'
                .format(self.id, self.requested_rate, len(self.paths)))


def parse_flows(ksp_xml_file_root):
    """Parse and build a flow object for each flow found in the KSP xml file.

    :param ksp_xml_file_root: The root element of the KSP xml file.
    :return: Dictionary with Flow Id as Key and Flow object as value.
    """
    flows = dict()  # type: Dict[int, Flow]

    for flow_element in ksp_xml_file_root.findall('FlowDetails/Flow'):
        if flow_element.get('Protocol') == 'A':  # Skip ACK flows
            continue

        flow = Flow(flow_element)

        if flow.id in flows:
            raise AssertionError('Flow id {} is duplicate'.format(flow.id))

        flows[flow.id] = flow

    return flows
