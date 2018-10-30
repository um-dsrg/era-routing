import math
import operator
from typing import Dict

import numpy as np
from lxml import etree

from classes.definitions import ACCURACY_VALUE
from classes.flow import Flow
from classes.link import Link


class Network:
    """Class representing the Network and its links.

    Attributes
    links:          A dictionary that stores the link's details.
                    Key: Link Id | Value: Link Object

    network_matrix: The network connection matrix is a matrix filled with
                    binary values that represents the paths each flow may take
                    to reach its destination. A row represents a path for a
                    particular flow and a column represents a link. A matrix
                    example is shown below:

                     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 < Link IDs
                    [1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0] # Path 0, Flow 0
                    [1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0] # Path 1, Flow 0
                    [0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1] # Path 2, Flow 1
                    [0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1] # Path 3, Flow 1
    """

    def __init__(self, ksp_xml_file_root, flows: Dict[int, Flow]):
        """Initialise the network object from the given xml file.

        :param ksp_xml_file_root: The root element of the KSP xml file.
        """
        self.links = dict()  # type: Dict[int, Link]
        self._generate_link_details(ksp_xml_file_root)

        self.network_matrix = self._create_network_matrix(flows)
        self._generate_network_matrix(flows)

        self.network_flow_upper_bound = \
            self._get_network_flow_upper_bound(flows)
        self.network_cost_upper_bound = \
            self._get_network_cost_upper_bound(flows)
        self.network_paths_upper_bound = self._get_network_paths_upper_bound()

    def get_link_capacity(self, link_id):
        """Return the capacity for the link with id link_id"""
        return self.links[link_id].capacity

    def get_num_paths(self):
        return self._get_network_paths_upper_bound()

    def append_to_xml(self, xml_root_element):
        """Append the network bounds to the XML result file."""
        network_element = etree.SubElement(xml_root_element, 'NetworkBounds')
        network_element.set('flow_upper_bound',
                            str(self.network_flow_upper_bound))
        network_element.set('cost_upper_bound',
                            str(self.network_cost_upper_bound))
        network_element.set('paths_upper_bound',
                            str(self.network_paths_upper_bound))

    def _create_network_matrix(self, flows: Dict[int, Flow]):
        """Create an all zero matrix with number of paths as rows and number
        of links as columns.

        :return: The all zero network matrix.
        """
        tot_num_paths = sum([flow.get_num_paths() for flow in flows.values()])
        return np.zeros((tot_num_paths, len(self.links)))

    def _generate_link_details(self, ksp_xml_file_root):
        """Parse the KSP XML file and populate the links dictionary."""
        for link_element in ksp_xml_file_root.findall('LinkDetails/Link'):
            link = Link(link_element)

            if link.id in self.links:
                raise AssertionError('Link {} is duplicate'.format(link.id))

            self.links[link.id] = link

        if not self.links:  # Error if no links are found
            raise RuntimeError('No links found in the xml file')

    def _generate_network_matrix(self, flows: Dict[int, Flow]):
        """Generate a binary matrix that represents the network.

        Generate a binary network matrix that sets a value of 1 in the location
        where a link is used by a certain path. This matrix is a representation
        of the network connectivity given the current flow set.

        :param flows: Dictionary of flows parsed from the KSP XML file.
        """
        for flow in flows.values():
            for path in flow.paths.values():
                path_id = path.id
                for link_id in path.links:
                    self.network_matrix[path_id, link_id] = 1

    def _get_network_cost_upper_bound(self, flows: Dict[int, Flow]):
        """Return the largest possible cost value with the given flow set."""
        total_network_cost = 0.0

        for flow in flows.values():
            remaining_data_rate = flow.requested_rate
            # Iterate through the paths in descending order
            for path in sorted(flow.get_paths(),
                               key=operator.attrgetter('cost'), reverse=True):
                min_link_capacity = min([self.links[link_id].capacity
                                        for link_id in path.links])

                if remaining_data_rate < min_link_capacity:
                    total_network_cost += remaining_data_rate * path.cost
                else:
                    total_network_cost += min_link_capacity * path.cost
                    remaining_data_rate -= min_link_capacity

                    if math.isclose(remaining_data_rate, 0,
                                    abs_tol=ACCURACY_VALUE):
                        break

        return math.ceil(total_network_cost)

    def _get_network_paths_upper_bound(self):
        """Returns the total number of paths in the network"""
        return self.network_matrix.shape[0]

    @staticmethod
    def _get_network_flow_upper_bound(flows: dict):
        """Return the total data rate requested."""
        return math.ceil(sum([flow.requested_rate for flow in flows.values()]))
