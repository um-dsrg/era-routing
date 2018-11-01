"""
Module that contains all the genetic algorithm operators.
mutation.
"""
import math
import operator
import random
from typing import List, Dict

import numpy as np

import modules.objectives as Objs
from .definitions import ACCURACY_VALUE, ACCURACY_ZERO_VALUE
from .flow import Flow
from .ga_statistics import OpType, GaStatistics
from .network import Network
from .parameters import Parameters
from .path import Path


class GaOperators:
    def __init__(self, flows: Dict[int, Flow], network: Network,
                 parameters: Parameters, ga_stats: GaStatistics, log_info):
        """Initialise the Genetic Algorithm Operators object

        :param flows:      Dictionary of the flows parsed from the KSP xml
                           file.
        :param network:    An instance of the Network object that has all the
                           information about the network and its properties.
        :param parameters: The parameters object that contains all the
                           necessary genetic algorithm parameters.
        :param ga_stats:   An instance of the GaStatistics class used to store
                           Genetic algorithm related statistics.
        :param log_info:   Pointer to a function that will insert a log entry
                           in the information log file.
        """
        self.flows = flows                     # type: Dict[int, Flow]
        self.network = network                 # type: Network
        self.current_operation = OpType.NO_OP  # type: OpType
        self.ga_stats = ga_stats               # type: GaStatistics
        self.log_info = log_info               # Information log file

        self.mutation_fraction = parameters.mutation_fraction
        # Get a list of functions that will be used to calculate the metric for
        # each of the objectives.
        self.metric_functions = \
            Objs.get_obj_metric_calc_fn(parameters.objectives, self)

    def generate_chromosome(self):
        """Generate a single chromosome.

        This function is called by the DEAP framework when generating the first
        population. This generation method will select the number of paths a
        flow is going to use at random with equal probability. In other words a
        flow can be transmitted over k paths, 1 paths or all the values in
        between with equal probability. For example, if k = 2, each flow can be
        either transmitted over a single path or both paths with equal
        probability.

        The chromosome size is equivalent to the total number of paths in the
        network.

        :return: A newly generated, valid chromosome
        """
        chromosome_size = self.network.get_num_paths()
        chromosome = [0] * chromosome_size

        for flow in self.flows.values():
            num_paths = flow.get_num_paths()
            paths_to_use = random.sample(list(flow.paths.values()),
                                         random.randint(1, num_paths))

            # Find the largest data rate we can transmit on the path
            for path in paths_to_use:
                path_link_capacities = [self.network.links[link_id].capacity
                                        for link_id in path.links]

                min_link_capacity = min(path_link_capacities)
                chromosome[path.id] = min(flow.requested_rate,
                                          min_link_capacity)

        return self._validate_chromosome(chromosome)

    def evaluate_chromosome(self, chromosome):
        """Calculate the chromosome's fitness.

        :param chromosome: The chromosome.

        :return: A tuple containing the fitness result for each objective
                 (weights). The objectives are the total network flow, total
                 network cost, and total number of paths being used. All values
                 returned are normalised such that they all fit within the
                 range of (0 <= x <= 1).
        """
        metric_values = [metric_function(chromosome)
                         for metric_function in self.metric_functions]

        self.log_info('Chromosme: {}\nMetrics: {}'
                      .format(chromosome, metric_values))

        return tuple(metric_values)
        # # NOTE This has been converted to a function
        # net_flow = sum(chromosome)  # Calculate the network flow

        # # NOTE This has been converted to a function
        # # Calculate the network cost
        # actual_network_matrix = (np.array(chromosome)[:, np.newaxis] *
        #                          self.network.network_matrix)

        # for link_id in range(actual_network_matrix.shape[1]):
        #     link = self.network.links[link_id]
        #     actual_network_matrix[:, link_id] *= link.cost

        # net_cost = np.sum(actual_network_matrix)

        # # NOTE This has been converted to a function
        # # Calculate number of paths used
        # paths_used = len([gene for gene in chromosome if gene > 0])

        # return (self._normalise_value(net_flow,
        #                               self.network.network_flow_upper_bound),
        #         self._normalise_value(net_cost,
        #                               self.network.network_cost_upper_bound),
        #         self._normalise_value(paths_used,
        #                               self.network.network_paths_upper_bound))

    def mate_chromosomes(self, chromosome_1, chromosome_2):
        """Perform the multi point crossover.

        Loop through each flow, and with a random probability flip the paths
        between chromosomes. This mimics multipoint crossover on a flow level.

        :param chromosome_1: Parent Chromosome 1
        :param chromosome_2: Parent Chromosome 2

        :return: A tuple of two child chromosomes generated by the crossover
                 operator.
        """
        self.current_operation = OpType.CROSSOVER
        self.ga_stats.log_operation(self.current_operation)

        random_split_ratio = random.random()  # Determine a random split ratio

        for flow in self.flows.values():
            if random.random() < random_split_ratio:  # Swap the flow usage
                flow_path_ids = flow.get_path_ids()

                for path_id in flow_path_ids:
                    temp = chromosome_1[path_id]
                    chromosome_1[path_id] = chromosome_2[path_id]
                    chromosome_2[path_id] = temp

        return (self._validate_link_capacity_constraint(chromosome_1),
                self._validate_link_capacity_constraint(chromosome_2))

    def mutate_chromosome(self, chromosome):
        """Mutate the chromosome with 1/3 probability of favoring one of the
        objectives.

        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        self.current_operation = OpType.MUTATION
        self.ga_stats.log_operation(self.current_operation)

        # Get the flows that will be affected by this mutation
        flows_to_mutate = self._get_flows_to_mutate()

        for flow in flows_to_mutate:
            for path_id in flow.get_path_ids():  # Reset the path
                chromosome[path_id] = 0

            rand_num = random.random()
            if rand_num < 0.33:  # Minimise the number of paths
                chromosome = self._min_path_mutation(flow, chromosome)
            elif rand_num < 0.66:  # Minimise the cost
                chromosome = self._min_cost_mutation(flow, chromosome)
            else:  # Maximise the flow
                chromosome = self._max_flow_mutation(flow, chromosome)

        # NOTE Always return a tuple
        return self._validate_chromosome(chromosome),

    def _get_flows_to_mutate(self) -> List[Flow]:
        """Return the flows to be mutated based on the mutation fraction."""
        num_flows = len(self.flows)
        num_flows_to_mutate = math.ceil(self.mutation_fraction * num_flows)

        return random.sample(list(self.flows.values()), num_flows_to_mutate)

    def _min_path_mutation(self, flow: Flow, chromosome):
        """Mutate the flow path usage to minimise the number of paths used.

        The number of paths to choose diminishes linearly as the number of
        paths increases. Transmission on no paths at all is possible and is
        given the highest probability value. The path's cost is not taken into
        consideration.

        :param flow:       The flow that will be mutated.
        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        num_paths = flow.get_num_paths()

        # List the number paths that can be used including 0
        path_options = list(range(num_paths + 1, 0, -1))
        total = sum(path_options)
        path_weights = [val/total for val in path_options]

        num_paths_mutate = np.random.choice(list(range(0, num_paths+1)), 1,
                                            p=path_weights)[0]

        if num_paths_mutate == 0:  # No paths are used
            return chromosome

        paths_to_mutate = random.sample(flow.get_paths(), num_paths_mutate)

        return self._max_flow_min_cost(flow, paths_to_mutate, chromosome)

    def _min_cost_mutation(self, flow: Flow, chromosome):
        """Mutate the flow path usage to minimise the cost.

        :param flow:       The flow that will be mutated.
        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        min_path_cost = min(flow.get_path_costs())

        paths_to_mutate = list()

        for path in flow.get_paths():
            if path.cost == min_path_cost:
                # 95% Probability smallest cost paths is chosen
                p_path_chosen = 0.95
            else:
                p_path_chosen = 0.95 * (min_path_cost / path.cost)

            if random.random() < p_path_chosen:
                paths_to_mutate.append(path)

        if paths_to_mutate:
            return self._max_flow_min_cost(flow, paths_to_mutate, chromosome)
        else:  # Return the chromosome as is if no paths are to be used
            return chromosome

    def _max_flow_mutation(self, flow: Flow, chromosome):
        """Mutate the flow path usage to maximise the allocated data rate.

        :param flow:       The flow that will be mutated.
        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        return self._max_flow_min_cost(flow, flow.get_paths(), chromosome)

    def _max_flow_min_cost(self, flow: Flow, paths_to_use: List[Path],
                           chromosome):
        """Assign the flow path usage to maximise flow at minimum cost using
        the given paths.

        Use the Maximum Flow Minimum Cost paradigm to assign as much data as
        possible over the paths with the minimum cost from the given path set.
        The current network usage is taken into consideration such that no
        links are over provisioned.

        :param flow:         The flow that will be routed.
        :param paths_to_use: List of paths to use.
        :param chromosome:   The chromosome to be modified.

        :return: The updated chromosome.
        """
        act_net_mat = (np.array(chromosome)[:, np.newaxis]
                       * self.network.network_matrix)

        # Get all the links the paths use and store their remaining capacity
        link_remaining_capacity = dict()  # Key: Link Id | Value: Capacity
        for path in paths_to_use:
            for link_id in path.links:
                if link_id not in link_remaining_capacity:
                    link_capacity = self.network.get_link_capacity(link_id)
                    link_usage = np.sum(act_net_mat[:, link_id])
                    link_remaining_capacity[link_id] = (link_capacity -
                                                        link_usage)

        remaining_data_rate = flow.requested_rate

        # Loop through paths sorted by cost
        for path in sorted(paths_to_use,
                           key=operator.attrgetter('cost')):
            min_remaining_capacity = min([link_remaining_capacity[link_id]
                                          for link_id in path.links])

            if remaining_data_rate < min_remaining_capacity:
                chromosome[path.id] = remaining_data_rate
                return chromosome
            else:
                chromosome[path.id] = min_remaining_capacity
                remaining_data_rate -= min_remaining_capacity

                for link_id in path.links:
                    link_remaining_capacity[link_id] -= min_remaining_capacity

        return chromosome

    def _validate_chromosome(self, chromosome):
        """Verify that the chromosome does not break any constraint.

        Checks that the chromosome does not break any of the constraints set.
        This function checks for flows that are receiving more data rate than
        what they requested and for over provisioned links in the given order.
        Flows are repaired first because it is futile to check for links
        exceeding capacity when there may be flows present that are allocated
        excess data rate.

        No changes are made to the chromosome if no constraints are broken.

        :param chromosome: The chromosome to be repaired.

        :return: A valid chromosome.
        """
        chromosome = self._validate_flow_constraint(chromosome)
        chromosome = self._validate_link_capacity_constraint(chromosome)

        return chromosome

    def _validate_flow_constraint(self, chromosome):
        """Check and repair any flows that are over provisioned.

        Check if there are any flows that are receiving more than their
        requested bandwidth. Flows that violate this constraint will have their
        data rate set equal to what they requested using the remove_excess
        function.

        :param chromosome: The chromosome.

        :return: The updated chromosome. If no violations are detected the
                 chromosome is not modified.
        """
        for flow in self.flows.values():
            data_per_path = [chromosome[path_id]
                             for path_id in flow.get_path_ids()]
            excess_flow = sum(data_per_path) - flow.requested_rate

            if excess_flow > 0:
                log_msg = ('Remove excess from flow: {}\nFlow Details\n{}'
                           'Excess Flow: {}\nFlow path BEFORE repair: {}\n'
                           .format(flow.id, flow, excess_flow, data_per_path))

                data_per_path = remove_excess(data_per_path, excess_flow)
                log_msg += 'Flow path AFTER repair: {}'.format(data_per_path)

                self.log_info(log_msg)  # Log the repair operation

                for index, path_id in enumerate(flow.paths):
                    chromosome[path_id] = data_per_path[index]

                # Log the operation
                self.ga_stats.log_flow_repair(self.current_operation)

        return chromosome

    def _validate_link_capacity_constraint(self, chromosome):
        """Check and repair any links that are over provisioned.

        Check if there are any links that are being over provisioned. If links
        that violate the capacity constraint are found, they will be repaired.
        The order of how links are repaired is random.

        :param chromosome: The chromosome

        :return: The updated chromosome. If no violations are detected the
                 chromosome is not be modified.
        """
        link_constraint_violated = True
        repairs_carried_out = False

        while link_constraint_violated is True:
            link_constraint_violated = False

            actual_network_matrix = (np.array(chromosome)[:, np.newaxis] *
                                     self.network.network_matrix)

            for link_id in random.sample(self.network.links.keys(),
                                         k=len(self.network.links)):
                link_column = actual_network_matrix[:, link_id]
                link_usage = np.sum(link_column)
                link_capacity = self.network.get_link_capacity(link_id)

                if link_usage > link_capacity:  # Over provisioned link found
                    repairs_carried_out = True
                    link_constraint_violated = True
                    excess_capacity = link_usage - link_capacity

                    log_msg = ('Repairing link: {}\nLink Capacity: {}\n'
                               'Link Usage: {}\nLink Excess: {}\n'
                               'Link Column BEFORE repair: {}\n'
                               .format(link_id, link_capacity, link_usage,
                                       excess_capacity, link_column))

                    non_zero_paths = [path_id for path_id, link_val in
                                      enumerate(link_column) if link_val > 0]
                    repaired_link = remove_excess(link_column, excess_capacity)

                    log_msg += ('Link Column AFTER repair {}'
                                .format(repaired_link))

                    self.log_info(log_msg)

                    for path_id in non_zero_paths:
                        chromosome[path_id] = repaired_link[path_id]

                    break

        if repairs_carried_out:  # Log when links are repaired
            self.ga_stats.log_link_repair(self.current_operation)

        return chromosome

    @staticmethod
    def _calculate_total_network_flow(chromosome):
        """Calculates the total network flow."""
        return sum(chromosome)

    @staticmethod
    def _calculate_total_paths_used(chromosome):
        """Calculates the total number of paths used."""
        return len([gene for gene in chromosome if gene > 0])

    def _calculate_total_network_cost(self, chromosome):
        """Calculates the total network cost."""
        actual_network_matrix = (np.array(chromosome)[:, np.newaxis] *
                                 self.network.network_matrix)

        for link_id in range(actual_network_matrix.shape[1]):
            link = self.network.links[link_id]
            actual_network_matrix[:, link_id] *= link.cost

        return np.sum(actual_network_matrix)

    @staticmethod
    def _normalise_value(value, max_value):
        """Normalise the value to have a range between 0 and 1.

        Checks are made to ensure that the normalised value is between 0 and 1.

        :param value:     The value to normalise.
        :param max_value: The largest amount the value can take. This parameter
                          is used to normalise the value variable with.

        :return: The normalised value.
        """
        normalised_value = value/max_value

        # Round very small numbers to 0
        if math.isclose(normalised_value, 0, abs_tol=ACCURACY_VALUE):
            normalised_value = 0.0

        if normalised_value < 0:  # Check for negative numbers
            raise AssertionError('Normalised value < 0.\nValue: {} '
                                 'Max Value: {} Normalised Value: {}'
                                 .format(value, max_value, normalised_value))

        # Set to 1, if the value is very close to 1
        if math.isclose(normalised_value, 1):
            normalised_value = 1.0

        if normalised_value > 1.0:
            raise AssertionError('Normalised value > 1.\nValue: {} '
                                 'Max Value: {} Normalised Value: {}'
                                 .format(value, max_value, normalised_value))

        return normalised_value


def remove_excess(values, excess):
    """
    Removes excess amount from values to make sum(values) equal to the desired
    limit.
    Normalise $values$ and $excess$ by dividing each element of $values$ and
    $excess$ by the sum of $values$.

    Define $x$ as the list containing the random generated numbers that signal
    the amount of capacity to remove from the link in the same location in the
    list link.
    Define $x_k$ as the kth element in $x$.
    Define $n$ as the number of elements in $values$.
    Define $k$ as the index of the current element being worked on.
    Define $z$ as the normalised $excess$.
    Define $y$ as the normalised $values$.
    Define $y_k$ as the kth element in $y$.

    For each element in $y$, $y_k$, a random number needs to be generated and
    stored in $x$, $x_k$, that represents the amount to remove from $y_k$.
    Limits on the minimum and maximum value that $x_k$ can take are imposed
    such that the excess is removed in one go. For example if we have [3, 1, 2]
    and an excess of 5, we need to remove at least 2 from $y_0$ (3) otherwise
    we cannot reach the excess quota of 5, even if we add $y_1$ and $y_2$
    (1 + 2 < 5).

    for all k in (0,n)

                      __ i = k - 1
               X  =  \            x
                     /__ i = 0     i

                      __ i = n
               Y  =  \            y
                     /__ i = k + 1 i

    max(0, (z - X - Y)) < =  x  < =  min(y , z - X)
                              k           k

    The relationship between $y_k$ and $x_k$ is shuffled such that $x_0$ will
    not always represent the amount to remove from $y_0$. This is done to
    increase the fairness of the algorithm.
    This algorithm is not 100% fair.

    :param values: A list of values that their sum exceed a specified limit by
                   excess.
    :param excess: The amount to remove such that:
                   sum(values) - excess = desired limit

    :return: A list of values that their sum does NOT exceed the desired limit.
    """
    total = sum(values)

    if excess > total:
        raise AssertionError('Excess exceeded total.\nTotal: {}\nExcess: {}\n'
                             'Values: {}'.format(total, excess, values))

    z = excess / total
    y = [v / total for v in values]
    n = len(values)
    x = []

    y_index_shuffled = list(range(0, len(y), 1))
    random.shuffle(y_index_shuffled)
    y = [y[i] for i in y_index_shuffled]

    for k in range(n - 1):
        # k used instead of k-1 because array slicing excludes the last value
        x_sum = sum(x[0:k])
        y_sum = sum(y[k + 1:n])

        min_value = max(0, z - x_sum - y_sum)
        max_value = min(y[k], z - x_sum)

        x.append(random.uniform(min_value, max_value))

    x.append(z - sum(x))

    # Sort the x values to match the order in the original values
    sorted_x = [0] * n

    for i, idx in enumerate(y_index_shuffled):
        sorted_x[idx] = x[i]

    repaired_values = [values[idx] - (sorted_x[idx] * total)
                       for idx in range(n)]

    for idx, value in enumerate(repaired_values):
        # Set very small numbers to zero
        if math.isclose(value, 0, abs_tol=ACCURACY_ZERO_VALUE):
            repaired_values[idx] = 0
        elif repaired_values[idx] < 0:  # Check for negative numbers
            raise AssertionError('Negative number found {}\nValue: {}\n'
                                 'Excess: {}'.format(value, values, excess))

    # Check that the repair operation succeeded
    if math.isclose((sum(repaired_values) - (sum(values) - excess)), 0,
                    abs_tol=ACCURACY_VALUE) is False:
        error_msg = ('Excess removal FAILED\nExcess: {}\nSum of values: {}\n'
                     'Sum of repair: {}\nDifference: {}'
                     .format(excess, sum(values), sum(repaired_values),
                             ((sum(values) - excess) - sum(repaired_values))))

        raise AssertionError(error_msg)

    return repaired_values