"""Module that has objectives related functionality."""
import argparse
from typing import List
from collections import namedtuple
from .ga_operators import GaOperators


OBJ_PROPERTIES = ['obj_name', 'obj_weight', 'fn_metric_calc', 'fn_obj_bound']
Objective = namedtuple('Objective', OBJ_PROPERTIES)


def objective(obj: str):
    """Return an Objective tuple, given a set of arguments.

    Checks are made in this function to make sure that the passed arguments are
    valid.

    :param obj: The objective in a comma delimited string.
    :type obj: str
    :return: The parsed objective in the Objective named tuple format.
    """
    split_obj = obj.split(',')
    split_obj = [text.strip() for text in split_obj]

    if len(split_obj) != len(OBJ_PROPERTIES):
        raise argparse.ArgumentTypeError('Each objective must have {} fields'
                                         .format(len(OBJ_PROPERTIES)))

    try:
        split_obj[1] = int(split_obj[1])
    except ValueError:
        raise argparse.ArgumentTypeError('The obj_weight needs to be an '
                                         'integer')

    parsed_obj = Objective._make(split_obj)

    if parsed_obj.obj_weight not in [-1, 1]:
        raise argparse.ArgumentTypeError('The obj_weight needs to be -1 or 1 '
                                         'ONLY.')

    try:
        getattr(GaOperators, parsed_obj.fn_metric_calc)
    except AttributeError:
        raise argparse.ArgumentTypeError('The function {} to calculate the '
                                         'metric does not exist'
                                         .format(parsed_obj.fn_metric_calc))

    return parsed_obj


def add_arg_to_parser(parser: argparse.ArgumentParser):
    """Add the objectives argument to the command line parser.

    :param parser: The parser object.
    :type parser: argparse.ArgumentParser
    :return: The updated parser.
    """
    parser.add_argument('--objectives', required=True, type=objective,
                        nargs='+')
    return parser


def get_obj_weights(objectives: List[Objective]):
    """Returns a tuple of the max/min objective weights."""
    return tuple([objective.obj_weight for objective in objectives])


def get_obj_names(objectives: List[Objective]):
    """Returns a list of the objective names."""
    return [objective.obj_name for objective in objectives]


def get_obj_metric_calc_fn(objectives: List[Objective]):
    """Returns a list of function pointers to calculate the metric value."""
    return [getattr(GaOperators, objective.fn_metric_calc)
            for objective in objectives]
