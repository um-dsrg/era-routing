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
        str_repr = ""

        if self.name == 'NO_OP':
            str_repr = 'No Operation'
        elif self.name == 'CROSSOVER':
            str_repr = 'Crossover'
        elif self.name == 'MUTATION':
            str_repr = 'Mutation'
        else:
            str_repr = "Undefined"

        return str_repr


@unique
class MutationType(Enum):
    """Represent the different Genetic Algorithm mutation operations possible.

    This enum will be used to log which mutation operation was carried out.
    This will be used to keep track of which mutation operations have the
    largest impact on the solution.
    """
    NO_OP = -1
    MIN_PATH = 0
    MIN_COST = 1
    MAX_FLOW = 2
    MIN_PATH_STD_DEV = 3
    ALL_RANDOM = 4

    def __repr__(self):
        return 'Enum MutationType(' + self.__str__() + ')'

    def __str__(self):
        str_repr = ""

        if self.name == 'MIN_PATH':
            str_repr = 'Minimise path usage'
        elif self.name == 'MIN_COST':
            str_repr = 'Minimise Cost'
        elif self.name == 'MIN_PATH_STD_DEV':
            str_repr = 'Minimise Path Standard Deviation'
        elif self.name == 'MAX_FLOW':
            str_repr = 'Maximise Flow'
        elif self.name == 'ALL_RANDOM':
            str_repr = 'All Random'
        elif self.name == "NO_OP":
            str_repr = "No Operation"
        else:
            str_repr = "Undefined"

        return str_repr


class MutationOpCounter:
    """Represent a counter associated with each mutation type"""

    def __init__(self):
        """Initialise all values to 0"""
        self.num_survived = 0
        self.num_carried_out = 0


class GaOpCounter:
    """Represent a counter for all the operators of the Genetic Algorithm."""

    def __init__(self):
        """Initialise all values to 0."""
        # # # Crossover # # #
        self.num_crossovers = 0  # The total number of crossovers performed

        # The number of chromosomes that were selected for the next generation
        # which had the crossover operator applied to them
        self.num_survived_crossovers = 0

        # # # Mutation # # #
        self.mutation_counter = {mut_type: MutationOpCounter() for mut_type
                                 in MutationType}  # type: Dict[MutationType, MutationOpCounter]

        # # # Repairs # # #
        self.n_flow_repaired_mutation = 0
        self.n_link_repaired_mutation = 0
        self.n_flow_repaired_crossover = 0
        self.n_link_repaired_crossover = 0


class GaStatistics:
    """Store statistics related to the Genetic Algorithm's operations.

    Stores the number of mutations, crossovers and their respective repaired
    fraction and the Genetic Algorithm's execution time.
    """

    def __init__(self, n_generations):
        self.xml_element = None

        self.current_generation = 0  # Used to store statistics by generation

        # Key: Generation Number | Value: GaOpCounter instance
        self.op_counter = {generation: GaOpCounter() for generation
                           in range(1, n_generations + 1)}  # type: Dict[int, GaOpCounter]

        self.nsga3_reference_points = None  # The list of reference points used by NSGA-3
        self.nsga3_reference_points_saved = False  # True when ref points are saved in the xml file

    def set_generation(self, generation):
        """Sets the current generation number."""
        self.current_generation = generation

    def log_crossover_operation(self) -> None:
        """Log a crossover operation"""
        self.op_counter[self.current_generation].num_crossovers += 1

    def log_mutation_operation(self, mutation_type: MutationType) -> None:
        """Log a mutation operation"""
        mutation_counter = self.op_counter[self.current_generation].mutation_counter
        mutation_counter[mutation_type].num_carried_out += 1

    def log_flow_repair(self, op_type: OpType):
        """Log that a flow has been repaired.

        :param op_type: The operation: CROSSOVER or MUTATION
        """
        if op_type == OpType.CROSSOVER:
            self.op_counter[self.current_generation].n_flow_repaired_crossover += 1
        elif op_type == OpType.MUTATION:
            self.op_counter[self.current_generation].n_flow_repaired_mutation += 1
        elif op_type == OpType.NO_OP:  # No counters need to be updated
            pass
        else:
            raise AssertionError('Invalid Operation type {}'.format(op_type))

    def log_link_repair(self, op_type: OpType):
        """Log that a link has been repaired.

        :param op_type: The operation: CROSSOVER or MUTATION
        """
        if op_type == OpType.CROSSOVER:
            self.op_counter[self.current_generation].n_link_repaired_crossover += 1
        elif op_type == OpType.MUTATION:
            self.op_counter[self.current_generation].n_link_repaired_mutation += 1
        elif op_type == OpType.NO_OP:  # No counters need to be updated
            pass
        else:
            raise AssertionError('Invalid Operation type {}'.format(op_type))

    def log_survivors(self, population: list) -> None:
        """Log the chromosomes that survived to the following generation

        Arguments:
            population {list} -- The chosen population
        """
        op_counter = self.op_counter[self.current_generation]
        for chromosome in population:
            # # # Crossover survivors # # #
            if chromosome.applied_crossover is True:
                op_counter.num_survived_crossovers += 1

            # # # Mutation survivors # # #
            if chromosome.mutation_operation != MutationType.NO_OP:
                op_counter.mutation_counter[chromosome.mutation_operation].num_survived += 1

    def log_nsga3_reference_points(self, reference_points: list):
        """Log the reference points used by the NSGA3 algorithm"""
        self.nsga3_reference_points = reference_points

    @staticmethod
    def reset_chromosome_counters(population: list) -> list:
        """Give each chromosome Mutation and crossover tracking attributes"""

        for chromosome in population:
            chromosome.applied_crossover = False
            chromosome.mutation_operation = MutationType.NO_OP

        return population

    def append_to_xml(self, xml_root):
        """Store the operation statistics results in XML format.

        :param xml_root: The root of the result XML file.
        """
        if self.xml_element is None:
            self.xml_element = etree.SubElement(xml_root, 'Statistics')

        if self.nsga3_reference_points_saved is False and self.nsga3_reference_points is not None:
            reference_points_element = etree.Element("ReferencePoints")
            reference_points_element.set("NumPoints", str(len(self.nsga3_reference_points)))

            for reference_point in self.nsga3_reference_points:
                reference_point_element = etree.SubElement(reference_points_element,
                                                           "ReferencePoint")
                reference_point_element.set("x", str(reference_point[0]))
                reference_point_element.set("y", str(reference_point[1]))
                reference_point_element.set("z", str(reference_point[2]))

            self.xml_element.append(reference_points_element)
            self.nsga3_reference_points_saved = True

        # Store the generations saved in the xml file to delete them once
        # written on disk.
        gen_to_delete = list()

        for generation, counter in self.op_counter.items():
            if generation <= self.current_generation:
                gen_element = etree.Element('Generation')
                gen_element.set('Id', str(generation))

                # # # Crossover # # #
                crossover_element = etree.SubElement(gen_element, 'Crossover')

                crossover_element.set("Total", str(counter.num_crossovers))
                crossover_element.set("Survived", str(counter.num_survived_crossovers))
                crossover_element.set("NumRepFlows", str(counter.n_flow_repaired_crossover))
                crossover_element.set("NumRepLinks", str(counter.n_link_repaired_crossover))

                # # # Mutation # # #
                mutation_element = etree.SubElement(gen_element, 'Mutation')

                tot_num_mutations = 0
                tot_num_survived_mutations = 0

                for mut_type, mut_counter in counter.mutation_counter.items():
                    if mut_type == MutationType.NO_OP:
                        continue  # Exclude NO_OP from the results

                    operator_element = etree.SubElement(mutation_element, "Operator")

                    tot_num_mutations += mut_counter.num_carried_out
                    tot_num_survived_mutations += mut_counter.num_survived

                    operator_element.set("Name", str(mut_type.name))
                    operator_element.set("Total", str(mut_counter.num_carried_out))
                    operator_element.set("Survived", str(mut_counter.num_survived))

                mutation_element.set("Total", str(tot_num_mutations))
                mutation_element.set("Survived", str(tot_num_survived_mutations))
                mutation_element.set("NumRepFlows", str(counter.n_flow_repaired_mutation))
                mutation_element.set("NumRepLinks", str(counter.n_link_repaired_mutation))

                self.xml_element.append(gen_element)
                gen_to_delete.append(generation)

        # Delete the generations that have been stored in the XML file
        for gen in gen_to_delete:
            del self.op_counter[gen]
