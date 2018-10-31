"""Set of tests to test the objectives."""
import unittest
import argparse
from ..classes.objectives import (add_arg_to_parser, get_obj_weights,
                                  get_obj_names)


class Objectives(unittest.TestCase):
    """Objectives"""

    def test_max_min(self):
        """Check that the max_min objective parameter is valid."""
        parser = argparse.ArgumentParser()
        parser = add_arg_to_parser(parser)

        with self.assertRaises(SystemExit):
            parser.parse_args(['--objectives',
                               'my_function, net_flow, -1',
                               'my_function2, net_cost, a'])

        with self.assertRaises(SystemExit):
            parser.parse_args(['--objectives',
                               'my_function, net_flow, -1',
                               'my_function2, net_cost, 2'])

        with self.assertRaises(SystemExit):
            parser.parse_args(['--objectives',
                               'my_function, net_flow, 0.1',
                               'my_function2, net_cost, 2'])

        with self.assertRaises(SystemExit):
            parser.parse_args(['--objectives',
                               'my_function, net_flow, -100',
                               'my_function2, net_cost, 1'])

    def test_objectives_not_passed(self):
        """Check that if the objectives are not passed, an error is shown."""
        parser = argparse.ArgumentParser()
        parser = add_arg_to_parser(parser)

        with self.assertRaises(SystemExit):
            parser.parse_args([''])

        with self.assertRaises(SystemExit):
            parser.parse_args(['--objective',
                               'my_function, net_flow, -100',
                               'my_function2, net_cost, 1'])

    def test_valid_config(self):
        """Test a valid configuration to make sure it works."""
        parser = argparse.ArgumentParser()
        parser = add_arg_to_parser(parser)
        args = parser.parse_args(['--objectives', 'my_function, net_flow, -1',
                                  'my_function2, net_cost, 1'])

        self.assertEqual(2, len(args.objectives))

        # Objective 1
        self.assertEqual('my_function', args.objectives[0].func_name)
        self.assertEqual('net_flow', args.objectives[0].obj_name)
        self.assertEqual(-1, args.objectives[0].max_min)

        # Objective 2
        self.assertEqual('my_function2', args.objectives[1].func_name)
        self.assertEqual('net_cost', args.objectives[1].obj_name)
        self.assertEqual(1, args.objectives[1].max_min)

    def test_obj_to_tuple(self):
        """Test the returned tuple containing the set weights."""
        parser = argparse.ArgumentParser()
        parser = add_arg_to_parser(parser)
        args = parser.parse_args(['--objectives', 'my_function, net_flow, -1',
                                  'my_function2, net_cost, 1'])

        obj_weights = get_obj_weights(args.objectives)
        self.assertSequenceEqual((-1, 1), obj_weights)

    def test_obj_name(self):
        """Test the returned list of objectives names."""
        parser = argparse.ArgumentParser()
        parser = add_arg_to_parser(parser)
        args = parser.parse_args(['--objectives', 'my_function, net_flow, -1',
                                  'my_function2, net_cost, 1'])

        obj_names = get_obj_names(args.objectives)
        self.assertSequenceEqual(['net_flow', 'net_cost'], obj_names)


if __name__ == '__main__':
    unittest.main(verbosity=2)
