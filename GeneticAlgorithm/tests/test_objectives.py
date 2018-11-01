"""Set of tests to test the objectives."""
import unittest
import argparse
from modules.objectives import (add_arg_to_parser, get_obj_weights,
                                get_obj_names)


class ObjectivesTestCase(unittest.TestCase):
    """Objectives"""

    def setUp(self):
        self.parser = argparse.ArgumentParser()
        self.parser = add_arg_to_parser(self.parser)

    def test_obj_weights(self):
        """Check that the objective weights passed are valid."""
        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives',
                                    'net_flow, -1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound',
                                    'net_flow, a, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound'])

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives',
                                    'net_flow, -1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound',
                                    'net_flow, 2, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound'])

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives',
                                    'net_flow, 0.1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound',
                                    'net_flow, 1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound'])

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives',
                                    'net_flow, -100, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound',
                                    'net_flow, 1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound'])

    def test_objectives_not_passed(self):
        """Check that if the objectives are not passed, an error is shown."""
        with self.assertRaises(SystemExit):
            self.parser.parse_args([''])

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objective',
                                    'dummy_string'])

    def test_valid_config(self):
        """Test a valid configuration to make sure it works."""
        args = self.parser.parse_args(['--objectives',
                                       'net_flow, -1, '
                                       '_calculate_total_network_flow, '
                                       '_get_network_flow_upper_bound',
                                       'net_cost, 1, '
                                       '_calculate_total_network_flow, '
                                       '_get_network_flow_upper_bound'])

        self.assertEqual(2, len(args.objectives))

        # Objective 1
        self.assertEqual('_calculate_total_network_flow',
                         args.objectives[0].fn_metric_calc)
        self.assertEqual('net_flow', args.objectives[0].obj_name)
        self.assertEqual(-1, args.objectives[0].obj_weight)

        # Objective 2
        self.assertEqual('_calculate_total_network_flow',
                         args.objectives[1].fn_metric_calc)
        self.assertEqual('net_cost', args.objectives[1].obj_name)
        self.assertEqual(1, args.objectives[1].obj_weight)

    def test_obj_to_tuple(self):
        """Test the returned tuple containing the set weights."""
        args = self.parser.parse_args(['--objectives',
                                       'net_flow, -1, '
                                       '_calculate_total_network_flow, '
                                       '_get_network_flow_upper_bound',
                                       'net_flow, 1, '
                                       '_calculate_total_network_flow, '
                                       '_get_network_flow_upper_bound'])

        obj_weights = get_obj_weights(args.objectives)
        self.assertSequenceEqual((-1, 1), obj_weights)

    def test_obj_name(self):
        """Test the returned list of objectives names."""
        args = self.parser.parse_args(['--objectives',
                                       'net_flow, -1, '
                                       '_calculate_total_network_flow, '
                                       '_get_network_flow_upper_bound',
                                       'net_cost, 1, '
                                       '_calculate_total_network_flow,'
                                       '_get_network_flow_upper_bound'])

        obj_names = get_obj_names(args.objectives)
        self.assertSequenceEqual(['net_flow', 'net_cost'], obj_names)

    def test_incorrect_num_obj_params(self):
        """Test exception is raised if incorrect number of params is passed."""
        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives',
                                    'net_flow, -1, '
                                    '_calculate_total_network_flow'])

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives',
                                    'net_flow, -1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound, extra'])

    def test_fn_obj_metric_calc(self):
        """Verify that the check to make sure the function exists works."""
        try:
            self.parser.parse_args(['--objectives', 'net_flow, 1, '
                                    '_calculate_total_network_flow,'
                                    '_get_network_flow_upper_bound'])
        except:
            self.fail('Exception raised when valid function name passed')

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives', 'net_flow, 1, '
                                    'non_existent_function, '
                                    '_get_network_flow_upper_bound'])

    def test_fn_obj_bound_calc(self):
        """Verify that the check to make sure the function exists works."""
        try:
            self.parser.parse_args(['--objectives', 'net_flow, 1, '
                                    '_calculate_total_network_flow, '
                                    '_get_network_flow_upper_bound'])
        except:
            self.fail('Exception raised when valid function name passed')

        with self.assertRaises(SystemExit):
            self.parser.parse_args(['--objectives', 'net_flow, 1, '
                                    '_calculate_total_network_flow, '
                                    'non_existent_function'])


if __name__ == '__main__':
    unittest.main(verbosity=2)
