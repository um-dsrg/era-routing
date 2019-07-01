"""Module that has objectives related functionality."""
from typing import List
from collections import namedtuple

from lxml import etree


class Objectives:
    obj_properties = ['obj_name', 'obj_weight', 'fn_metric_calc', 'fn_obj_bound']
    Objective = namedtuple('Objective', obj_properties)

    def __init__(self, objs: List[str]):
        """Initialise the objectives from a list of strings.

        :param objs: List of objectives in string format.
        """
        self.objectives = list()  # type: List[Objective]
        self._parse_objectives(objs)

    def append_to_xml(self, xml_root_element):
        """Append the Objectives details to the XML result file."""
        objs_element = etree.SubElement(xml_root_element, 'Objectives')

        for objective in self.objectives:
            obj_element = etree.SubElement(objs_element, 'Objective')
            obj_element.set('name', str(objective.obj_name))
            obj_element.set('weight', str(objective.obj_weight))
            obj_element.set('fn_metric_calc', str(objective.fn_metric_calc))
            obj_element.set('fn_obj_bound', str(objective.fn_obj_bound))

    def gen_num_objectives(self):
        """Return the number of objectives"""
        return len(self.objectives)

    def get_obj_weights(self):
        """Return a tuple of the max/min objective weights."""
        return tuple([objective.obj_weight for objective in self.objectives])

    def get_obj_names(self):
        """Return a list of the objective names."""
        return [objective.obj_name for objective in self.objectives]

    def get_metric_calc_fns(self):
        """Return a list of the function names to calculate the metric value."""
        return [objective.fn_metric_calc for objective in self.objectives]

    def get_obj_bound_fns(self):
        """Return a list of the function names to calculate the metric bound."""
        return [objective.fn_obj_bound for objective in self.objectives]

    def _parse_objectives(self, objectives):
        for objective in objectives:
            split_obj = objective.split(',')
            split_obj = [text.strip() for text in split_obj]

            if len(split_obj) != len(Objectives.obj_properties):
                raise RuntimeError('Each objective must have {} fields.'
                                   .format(len(Objectives.obj_properties)))

            try:
                obj_weight = int(split_obj[1])
                if obj_weight not in [-1, 1]:
                    raise RuntimeError('The obj_weight needs to be -1 or 1 ONLY.')
                split_obj[1] = obj_weight
            except ValueError:
                print('The obj_weight needs to be an integer.')
                raise

            # Create the named tuple from list of values
            parsed_obj = Objectives.Objective._make(split_obj)
            self.objectives.append(parsed_obj)
