"""Test the metric calculation methods"""
import unittest
import copy

from modules.ga_operators import GaOperators
from modules.flow import Flow
from modules.xml_handler import XmlHandler

class MockParameters:
    mutation_fraction = 0

class MockObjectives:
    def get_metric_calc_fns(self):
        return []

class MetricCalcTestSuite(unittest.TestCase):

    def setUp(self):
        mock_param = MockParameters()
        mock_objs = MockObjectives()

        # Parse the KSP xml file to build the flows dictionary
        ksp_xml = XmlHandler('tests/butterfly_ksp.xml')
        flows = Flow.parse_flows(ksp_xml.get_root())

        self.ga_ops = GaOperators(flows, None, mock_param, mock_objs, None, None)

    def test_total_flow_metric(self):
        """Check total flow metric"""
        chromosome = [1.4, 2.2, 1.0, 100]
        self.assertEqual(self.ga_ops._calculate_total_network_flow(chromosome),
                         sum(chromosome))

    def test_flow_splits_metric(self):
        """Check flow splits metric"""
        # No paths are used
        chromosome = [0, 0, 0, 0]
        self.assertEqual(self.ga_ops._calculate_flow_splits_metric(chromosome), 0)

        # All paths being used
        chromosome = [1, 1, 1, 1]
        self.assertAlmostEqual(self.ga_ops._calculate_flow_splits_metric(chromosome), 2.67, places=2)

        # All paths being used (check that the data rate does not affect the metric value)
        chromosome = [4.4, 1.11, 10.4, 100.1]
        self.assertAlmostEqual(self.ga_ops._calculate_flow_splits_metric(chromosome), 2.67, places=2)

        # One flow being split
        chromosome = [1, 0, 1, 1]
        self.assertAlmostEqual(self.ga_ops._calculate_flow_splits_metric(chromosome), 1.33, places=2)

        # No flows being split
        chromosome = [1.2, 0, 0, 5.2]
        self.assertEqual(self.ga_ops._calculate_flow_splits_metric(chromosome), 0)

    def test_delay_distribution_metric(self):
        """Check delay distribution metric"""
        chromosome = [0, 0, 0, 0]
        self.assertEqual(self.ga_ops._calculate_delay_distribution_metric(chromosome), 0)

        # All data rate transmitted on the path with lowest delay value
        chromosome = [5, 0, 1, 0]
        self.assertEqual(self.ga_ops._calculate_delay_distribution_metric(chromosome), 2)

        # Same as above but with real values (to check that the data rate does not have an affect on the delay metric)
        chromosome = [0.01, 0, 0.2, 0]
        self.assertEqual(self.ga_ops._calculate_delay_distribution_metric(chromosome), 2)

        # Flow using two paths, the other using the lowest delay path
        chromosome = [5, 10, 1, 0]
        self.assertAlmostEqual(self.ga_ops._calculate_delay_distribution_metric(chromosome), 1.56, places=2)

        # All paths being used
        chromosome = [4, 1.5, 2.5, 3.7]
        self.assertAlmostEqual(self.ga_ops._calculate_delay_distribution_metric(chromosome), 1.42, places=2)

        mod_ga_ops = copy.deepcopy(self.ga_ops)  # Create a copy so we do not modify the original

        # Set all paths to have the same cost
        for flow in mod_ga_ops.flows.values():
            for path in flow.paths.values():
                path.cost = 12.34

        chromosome = [1.2, 2.1, 3.5, 1.1]
        self.assertEqual(mod_ga_ops._calculate_delay_distribution_metric(chromosome), 2)

        # Set the paths of only one flow to have equal cost
        path_delay = [12.2, 12.2, 4.1, 11.3]
        for flow in mod_ga_ops.flows.values():
            for path in flow.paths.values():
                path.cost = path_delay.pop(0)

        chromosome = [2.2, 1.5, 2.6, 0.1]
        self.assertAlmostEqual(mod_ga_ops._calculate_delay_distribution_metric(chromosome), 1.97, places=2)

        # Set unequal number of paths, all paths identical delay value
        flow_3 = mod_ga_ops.flows[3]
        flow_3.paths[4] = copy.deepcopy(flow_3.paths[2])
        flow_3.paths[4].id = 4
        flow_3.paths[5] = copy.deepcopy(flow_3.paths[2])
        flow_3.paths[5].id = 5

        path_delay = [2, 2, 1, 1, 1, 1]
        for flow in mod_ga_ops.flows.values():
            for path in flow.paths.values():
                path.cost = path_delay.pop(0)

        chromosome = [2.2, 1.5, 2.6, 0.1, 0.1, 1.1]
        self.assertEqual(mod_ga_ops._calculate_delay_distribution_metric(chromosome), 2)

        # Set unequal number of paths, where paths have different delay values and incorrect order
        path_delay = [1, 12, 5, 6, 2, 10]
        for flow in mod_ga_ops.flows.values():
            for path in flow.paths.values():
                path.cost = path_delay.pop(0)

        chromosome = [0.1, 1.1, 2.2, 1.5, 2.6, 0.1]
        self.assertAlmostEqual(mod_ga_ops._calculate_delay_distribution_metric(chromosome), 0.70, places=2)
