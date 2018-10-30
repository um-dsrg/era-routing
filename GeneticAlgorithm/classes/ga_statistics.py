from enum import Enum, unique
from typing import Dict

from lxml import etree


@unique
class OpType(Enum):
    """Represent the different Genetic Algorithm operations.

    NO_OP is used when no mutation/crossover have taken place yet. For example
    when generating the first population no mutation/crossover has yet taken
    place but the validate functions may still be called.
    """
    NO_OP = -1
    CROSSOVER = 0
    MUTATION = 1

    def __repr__(self):
        return 'Enum OpType(' + self.__str__() + ')'

    def __str__(self):
        if self.name == 'NO_OP':
            return 'No Operation'
        elif self.name == 'CROSSOVER':
            return 'Crossover'
        elif self.name == 'MUTATION':
            return 'Mutation'


class GaOpCounter:
    """Represent a counter for all the operators of the Genetic Algorithm."""

    def __init__(self):
        """Initialise all values to 0."""
        self.n_crossover = 0
        self.n_mutation = 0
        self.n_flow_repaired_crossover = 0
        self.n_link_repaired_crossover = 0
        self.n_flow_repaired_mutation = 0
        self.n_link_repaired_mutation = 0


class GaStatistics:
    """Store statistics related to the Genetic Algorithm's operations.

    Stores the number of mutations, crossovers and their respective repaired
    fraction and the Genetic Algorithm's execution time.
    """

    def __init__(self, n_generations):
        self.current_generation = 0  # Used to store statistics by generation
        self.xml_element = None

        # Key: Generation Number | Value: GaOpCounter instance
        self.op_counter = \
            {generation: GaOpCounter()
             for generation
             in range(1, n_generations + 1)}  # type: Dict[int, GaOpCounter]

    def set_generation(self, generation):
        """Sets the current generation number."""
        self.current_generation = generation

    def log_operation(self, op_type: OpType):
        """Log an operation.

        The operation can be either a crossover or a mutation. The counter is
        used to determine the number of repairs carried out compared to the
        total number of crossover/mutations that occurred in the Genetic
        Algorithm run.

        :param op_type: The operation: CROSSOVER or MUTATION
        """
        if op_type == OpType.CROSSOVER:
            self.op_counter[self.current_generation].n_crossover += 1
        elif op_type == OpType.MUTATION:
            self.op_counter[self.current_generation].n_mutation += 1
        else:
            raise AssertionError('Invalid Operation type {}'.format(op_type))

    def log_flow_repair(self, op_type: OpType):
        """Log that a flow has been repaired.

        :param op_type: The operation: CROSSOVER or MUTATION
        """
        if op_type == OpType.CROSSOVER:
            self.op_counter[
                self.current_generation].n_flow_repaired_crossover += 1
        elif op_type == OpType.MUTATION:
            self.op_counter[
                self.current_generation].n_flow_repaired_mutation += 1
        elif op_type == OpType.NO_OP:  # No counters need to be updated
            pass
        else:
            raise AssertionError('Invalid Operation type {}'.format(op_type))

    def log_link_repair(self, op_type: OpType):
        """Log that a link has been repaired.

        :param op_type: The operation: CROSSOVER or MUTATION
        """
        if op_type == OpType.CROSSOVER:
            self.op_counter[
                self.current_generation].n_link_repaired_crossover += 1
        elif op_type == OpType.MUTATION:
            self.op_counter[
                self.current_generation].n_link_repaired_mutation += 1
        elif op_type == OpType.NO_OP:  # No counters need to be updated
            pass
        else:
            raise AssertionError('Invalid Operation type {}'.format(op_type))

    def append_to_xml(self, xml_root):
        """Store the operation statistics results in XML format.

        :param xml_root: The root of the result XML file.
        """
        if self.xml_element is None:
            self.xml_element = etree.SubElement(xml_root, 'Statistics')

        # Store the generations saved in the xml file to delete them once
        # written on disk.
        gen_to_delete = list()

        for generation, counter in self.op_counter.items():
            if generation <= self.current_generation:
                gen_element = etree.Element('Generation')
                gen_element.set('Id', str(generation))

                # # # Crossover # # #
                crossover_element = etree.SubElement(gen_element, 'Crossover')
                crossover_element.set('n', str(counter.n_crossover))

                crossover_element.set('n_repaired_flows',
                                      str(counter.n_flow_repaired_crossover))

                crossover_element.set('n_repaired_links',
                                      str(counter.n_link_repaired_crossover))

                # # # Mutation # # #
                mutation_element = etree.SubElement(gen_element, 'Mutation')

                mutation_element.set('n', str(counter.n_mutation))

                mutation_element.set('n_repaired_flows',
                                     str(counter.n_flow_repaired_mutation))

                mutation_element.set('n_repaired_links',
                                     str(counter.n_link_repaired_mutation))

                self.xml_element.append(gen_element)
                gen_to_delete.append(generation)

        # Delete the generations that have been stored in the XML file
        for gen in gen_to_delete:
            del self.op_counter[gen]
