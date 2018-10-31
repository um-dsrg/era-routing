"""Module that has objectives related functionality."""
import argparse
from typing import List
from collections import namedtuple


Objective = namedtuple('Objective', ['func_name', 'obj_name', 'max_min'])


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

    try:
        split_obj[2] = int(split_obj[2])
    except ValueError:
        raise argparse.ArgumentTypeError('The max_min objective needs to be '
                                         'an integer')

    parsed_obj = Objective._make(split_obj)

    if parsed_obj.max_min not in [-1, 1]:
        raise argparse.ArgumentTypeError('The max_min objectives needs to be '
                                         '-1 or 1 ONLY.')

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
    return tuple([objective.max_min for objective in objectives])


def get_obj_names(objectives: List[Objective]):
    """Returns a list of the objective names."""
    return [objective.obj_name for objective in objectives]
