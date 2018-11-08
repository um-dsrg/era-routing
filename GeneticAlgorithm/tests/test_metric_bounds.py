"""Test the metric bounds calculation"""
import unittest

from modules.network import Network
from modules.flow import Flow
from modules.xml_handler import XmlHandler


class MockObjectives:
    def get_obj_names(self):
        return []

    def get_obj_bound_fns(self):
        return []


class BoundsTestSuite(unittest.TestCase):

    def setUp(self):
        # Parse the KSP xml file to build the flows dictionary
        ksp_xml = XmlHandler('tests/butterfly_ksp.xml')
        mock_objs = MockObjectives()

        self.flows = Flow.parse_flows(ksp_xml.get_root())
        self.network = Network(ksp_xml.get_root(), self.flows, mock_objs)

    def test_flow_upper_bound(self):
        """Test the flow upper bound"""
        self.assertEqual(Network._get_network_flow_upper_bound(self.flows), 20)

    def test_delay_distribution_upper_bound(self):
        """Test the delay distribution upper bound"""
        self.assertEqual(self.network._get_delay_distribution_upper_bound(self.flows), 2)

    def test_flow_split_upper_bound(self):
        """Test the flow splut upper bound"""
        self.assertEqual(self.network._get_flow_splits_upper_bound(self.flows), 3)
