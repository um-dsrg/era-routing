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

    def __repr__(self):
        return 'Enum MutationType(' + self.__str__() + ')'

    def __str__(self):
        if self.name == 'MIN_PATH':
            return 'Minimise path usage'
        elif self.name == 'MIN_COST':
            return 'Minimise Cost'
        elif self.name == 'MIN_PATH_STD_DEV':
            return 'Minimise Path Standard Deviation'
        elif self.name == 'MAX_FLOW':
            return 'Maximise Flow'
        elif self.name == "NO_OP":
            return "No Operation"


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


class MutationTypeCounter:

    def __init__(self, mut_operator_name, mut_carried_out, mut_survived):
        if mut_survived > mut_carried_out:
            raise AssertionError("The number of survived mutations is bigger than the mutations carried out")

        self.mutation_op_name = mut_operator_name
        self.mutations_carried_out = mut_carried_out
        self.mutations_survived = mut_survived


class GaStatistics:
    """Store statistics related to the Genetic Algorithm's operations.

    Stores the number of mutations, crossovers and their respective repaired
    fraction and the Genetic Algorithm's execution time.
    """

    def __init__(self, n_generations):
        self.current_generation = 0  # Used to store statistics by generation
        self.xml_element = None

        self.mutation_survival_counter = dict()

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

    def log_survived_mutation(self, population: list, off_mut_type_counter: dict) -> None:
        """
        Log the mutation operators that survived. This function needs to be€
        called after every generation to log the process on a per generation
        basis.

        :param population: The population after the selection process.
        :param off_mut_type_counter: The mutation counter for the generated
                                     offspring.
        :return: None
        """
        # The mutation counter for the population
        pop_mut_type_counter = GaStatistics.count_mutation_operations(population)

        mutation_counters = list()
        for mutation_operator, offspring_counter in off_mut_type_counter.items():
            pop_counter = pop_mut_type_counter[mutation_operator]
            mutation_counters.append(MutationTypeCounter(mutation_operator, offspring_counter, pop_counter))

        self.mutation_survival_counter[self.current_generation] = mutation_counters

    @staticmethod
    def reset_mutation_operations(population: list) -> None:
        for chromosome in population:
            if hasattr(chromosome, "mutation_type"):
                chromosome.mutation_type = MutationType.NO_OP

    @staticmethod
    def count_mutation_operations(population: list) -> dict:
        # A counter that stores the type of mutations carried out for a given
        # generation.
        mut_type_counter = {mutation_type: 0 for mutation_type in MutationType}

        for chromosome in population:
            if hasattr(chromosome, "mutation_type"):
                if chromosome.mutation_type != MutationType.NO_OP:
                    mut_type_counter[chromosome.mutation_type] += 1

        return mut_type_counter

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

                # # # Mutation Operation Survival # # #
                mutation_survival = etree.SubElement(gen_element, "MutationSurvival")
                mutation_counters = self.mutation_survival_counter[generation]
                for mut_counter in mutation_counters:
                    if mut_counter.mutation_op_name == MutationType.NO_OP:
                        continue  # Exclude the NO_OP from the results

                    operator_element = etree.SubElement(mutation_survival, "Operator")
                    operator_element.set("Name", str(mut_counter.mutation_op_name.name))
                    operator_element.set("Survived", str(mut_counter.mutations_survived))
                    operator_element.set("Total", str(mut_counter.mutations_carried_out))

        # Delete the generations that have been stored in the XML file
        for gen in gen_to_delete:
            del self.op_counter[gen]
