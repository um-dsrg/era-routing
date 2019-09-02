"""
Module that contains all the genetic algorithm operators.
"""
import math
import random
import statistics
from typing import NamedTuple

import numpy as np

from modules.definitions import ACCURACY_VALUE, ACCURACY_ZERO_VALUE
from modules.ga_statistics import OpType, MutationType


class MutationFunction(NamedTuple):
    """
    A custom Namedtuple that will hold the probability and function pointer to
    that mutation function
    """
    probability: float
    function


class GaOperators:
    def __init__(self, flows, network, parameters, objectives, ga_stats,
                 log_info):
        # type: (Dict[int, Flow], Network, Parameters, GaStatistics, Any)
        """Initialise the Genetic Algorithm Operators object

        :param flows:      Dictionary of the flows parsed from the KSP xml
                           file.
        :param network:    An instance of the Network object that has all the
                           information about the network and its properties.
        :param parameters: The parameters object that contains all the
                           necessary genetic algorithm parameters.
        :param objectives: The objectives object. Used to extract the metric
                           calculation functions.
        :param ga_stats:   An instance of the GaStatistics class used to store
                           Genetic algorithm related statistics.
        :param log_info:   Pointer to a function that will insert a log entry
                           in the information log file.
        """
        self.flows = flows  # type: Dict[int, Flow]
        self.network = network  # type: Network
        self.current_operation = OpType.NO_OP  # type: OpType
        self.ga_stats = ga_stats  # type: GaStatistics
        self.log_info = log_info  # Information log file

        self.mutation_fraction = parameters.mutation_fraction

        cumulativeProbability = 0.0
        self.mutationFunctions = list()

        for probability, functionName in zip(parameters.mutationFunctionProbability,
                                             parameters.mutationFunctions):
            cumulativeProbability += probability
            self.mutationFunctions.append(MutationFunction(cumulativeProbability,
                                                           getattr(self,
                                                                   F"mutation_{functionName}")))

        # Get a list of functions that will be used to calculate the metric for
        # each of the objectives.
        self.metric_functions = [getattr(self, metric_function_name)
                                 for metric_function_name
                                 in objectives.get_metric_calc_fns()]

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
        self.log_info('Chromosome: {} | Metrics: {}'.format(chromosome, metric_values))

        obj_bounds = self.network.obj_bound_values
        normalised_values = [self._normalise_value(metric_value, obj_bound)
                             for metric_value, obj_bound
                             in zip(metric_values, obj_bounds)]

        self.log_info('Obj bounds: {}'.format(obj_bounds))
        self.log_info('Normalised Metrics: {}'.format(normalised_values))

        return tuple(normalised_values)

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
        self.ga_stats.log_crossover_operation()

        random_split_ratio = random.random()  # Determine a random split ratio
        self.log_info("Crossover with split ratio of {}".format(random_split_ratio))

        for flow in self.flows.values():
            if random.random() < random_split_ratio:  # Swap the flow usage
                flow_path_ids = flow.get_path_ids()

                for path_id in flow_path_ids:
                    temp = chromosome_1[path_id]
                    chromosome_1[path_id] = chromosome_2[path_id]
                    chromosome_2[path_id] = temp

        # # # Crossover tracking # # #
        chromosome_1.applied_crossover = True
        chromosome_2.applied_crossover = True

        return (self._validate_link_capacity_constraint(chromosome_1),
                self._validate_link_capacity_constraint(chromosome_2))

    def mutate_chromosome(self, chromosome):
        """Perform the mutation operator on the chromosome

        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        self.current_operation = OpType.MUTATION

        # Get the flows that will be affected by this mutation
        flows_to_mutate = self._get_flows_to_mutate()

        for flow in flows_to_mutate:
            for path_id in flow.get_path_ids():  # Reset the path
                chromosome[path_id] = 0

            rand_num = random.random()
            if rand_num < 0.25:  # Minimise the number of paths
                self.log_info("Mutation: {} | {}".format(rand_num, MutationType.MIN_PATH))
                chromosome = self._min_path_mutation(flow, chromosome)
                chromosome.mutation_operation = MutationType.MIN_PATH
            elif rand_num < 0.50:  # Minimise the cost
                self.log_info("Mutation: {} | {}".format(rand_num, MutationType.MIN_COST))
                chromosome = self._min_cost_mutation(flow, chromosome)
                chromosome.mutation_operation = MutationType.MIN_COST
            elif rand_num < 0.75:  # Minimise the path standard deviation
                self.log_info("Mutation: {} | {}".format(rand_num, MutationType.MIN_PATH_STD_DEV))
                chromosome = self._min_path_std_dev_mutation(flow, chromosome)
                chromosome.mutation_operation = MutationType.MIN_PATH_STD_DEV
            else:  # Maximise the flow
                self.log_info("Mutation: {} | {}".format(rand_num, MutationType.MAX_FLOW))
                chromosome = self._max_flow_mutation(flow, chromosome)
                chromosome.mutation_operation = MutationType.MAX_FLOW

        self.ga_stats.log_mutation_operation(chromosome.mutation_operation)
        return self._validate_chromosome(chromosome),  # NOTE Always return a tuple

    def round_small_numbers(self, population: list) -> list:
        """Rounds very small numbers to zero in a given population

        Arguments:
            population {list} -- The population to check for negative numbers

        Raises:
            AssertionError: Raised when a negative number is found

        Returns:
            list -- The new population with very small numbers rounded to zero
        """
        self.log_info("The popluation before rounding: {}".format(population))

        for chromosome_index, chromosome in enumerate(population):
            for gene_index, gene in enumerate(chromosome):
                if gene != 0 and math.isclose(gene, 0, abs_tol=ACCURACY_ZERO_VALUE):
                    population[chromosome_index][gene_index] = 0
                    self.log_info("Chromosome {} Gene {} is rounded to 0 from {}"
                                  .format(chromosome_index, gene_index, gene))
                elif gene < 0:
                    raise AssertionError("Chromosome {} Gene {} has a negative value of {}"
                                         .format(chromosome_index, gene_index, gene))

        self.log_info("The population after rounding: {}".format(population))

        return population

    def _get_flows_to_mutate(self) -> list:
        """Return the flows to be mutated based on the mutation fraction."""
        num_flows = len(self.flows)
        num_flows_to_mutate = math.ceil(self.mutation_fraction * num_flows)

        return random.sample(list(self.flows.values()), num_flows_to_mutate)

    def _min_path_mutation(self, flow, chromosome):
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
        path_weights = [val / total for val in path_options]

        num_paths_mutate = np.random.choice(list(range(0, num_paths + 1)), 1, p=path_weights)[0]

        if num_paths_mutate == 0:  # No paths are used
            return chromosome

        paths_to_mutate = random.sample(flow.get_paths(), num_paths_mutate)

        return self._assign_data_rate_on_paths(flow, paths_to_mutate, chromosome)

    def _min_cost_mutation(self, flow, chromosome):
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
            return self._assign_data_rate_on_paths(flow, paths_to_mutate, chromosome)
        # else Return the chromosome as is if no paths are to be used
        return chromosome

    def _min_path_std_dev_mutation(self, flow, chromosome):
        """Mutate the flow path usage to minimise the path standard deviation.

        :param flow:       The flow that will be mutated.
        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        self.log_info('_min_path_std_dev_mutation - Mutating Flow: {} | Paths: {}'
                      .format(flow.id, flow.get_paths()))

        paths_to_mutate = list()

        flow_paths = flow.get_paths()

        if len(flow_paths) > 1:
            # Choose a path at random to use as the base path. All comparisons will
            # be made against this path.
            base_path = random.choice(flow_paths)

            # Find the largest gap between the chosen path and all the other paths
            largest_cost_difference = max([abs(path.cost - base_path.cost)
                                           for path in flow_paths if path.id != base_path.id])

            self.log_info('_min_path_std_dev_mutation - Base Path: {} | Largest Cost Difference: {}'
                          .format(base_path, largest_cost_difference))

            for path in flow_paths:
                if path.id == base_path.id:  # The base path must always be included
                    paths_to_mutate.append(path)
                else:
                    p_choose_path = 1 - (abs(base_path.cost - path.cost) /
                                         float(largest_cost_difference + 1))
                    self.log_info('_min_path_std_dev_mutation - Probability to choose path: {} cost {} is : {}'
                                  .format(path.id, path.cost, p_choose_path))

                    random_number = random.random()
                    if random_number < p_choose_path:
                        self.log_info('_min_path_std_dev_mutation - Random Number: {} | Path {} added to mutation list'
                                      .format(random_number, path.id))
                        paths_to_mutate.append(path)
        else:
            paths_to_mutate.append(flow_paths[0])  # Add the only path available to that flow
            self.log_info('_min_path_std_dev_mutation - Path {} added to mutation list because it is the only path'
                          .format(paths_to_mutate[0].id))

        return self._assign_data_rate_on_paths(flow, paths_to_mutate, chromosome)

    def _max_flow_mutation(self, flow, chromosome):
        """Mutate the flow path usage to maximise the allocated data rate.

        :param flow:       The flow that will be mutated.
        :param chromosome: The chromosome to be mutated.

        :return: The mutated chromosome.
        """
        return self._assign_data_rate_on_paths(flow, flow.get_paths(), chromosome)

    def _assign_data_rate_on_paths(self, flow, paths_to_use, chromosome):
        """Assign data rate on the give path set for a particular flow

        The function will try to maximise flow assignment on the given path set.
        The order in which paths are selected is randomised. The current network
        usage is taken into consideration such that no links are over
        provisioned.

        :param flow:         The flow that will be routed.
        :param paths_to_use: List of paths to use.
        :param chromosome:   The chromosome to be modified.

        :return: The updated chromosome.
        """
        act_net_mat = (np.array(chromosome)[:, np.newaxis] * self.network.network_matrix)

        # Get all the links the paths use and store their remaining capacity
        link_remaining_capacity = dict()  # Key: Link Id | Value: Capacity
        for path in paths_to_use:
            for link_id in path.links:
                if link_id not in link_remaining_capacity:
                    link_capacity = self.network.get_link_capacity(link_id)
                    link_usage = np.sum(act_net_mat[:, link_id])
                    link_remaining_capacity[link_id] = (link_capacity - link_usage)

        remaining_data_rate = flow.requested_rate

        # Loop through the shuffled paths and assign data rate accordingly
        random.shuffle(paths_to_use)
        for path in paths_to_use:
            min_remaining_capacity = min([link_remaining_capacity[link_id]
                                          for link_id in path.links])

            if remaining_data_rate < min_remaining_capacity:
                chromosome[path.id] = remaining_data_rate
                break
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

                for ack_path in self.network.get_ack_paths_used_by_link(link_id):
                    # The below calculation assumes an ACK packet tranmitted
                    # every 2 Data packets received
                    link_usage += (chromosome[ack_path] * 0.0458)

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

    def _calculate_flow_splits_metric(self, chromosome):
        """Calculates the flow splits metric

        The flow splits metric is split into two sections; the integer
        component and the fractional component. The integer component is
        equal to the number of flows that have a split. The fractional
        component is equal to the total number of flow splits. The fractional
        component is normalised to take a range between 0 and 1.

        :param chromosome: The chromosome to be evaluated.
        :return: The flow splits metric.
        """
        max_flow_splits = 0  # The maximum number of flow splits possible
        total_flow_splits = 0  # The total number of flow splits
        num_flows_with_split = 0  # The toal number of flows that have a split

        for flow in self.flows.values():
            used_paths = flow.get_path_ids()
            max_flow_splits += len(used_paths) - 1
            data_on_paths = [chromosome[path_id] for path_id in used_paths
                             if chromosome[path_id] > 0]

            num_used_paths = len(data_on_paths)

            if num_used_paths > 1:
                num_flows_with_split += 1

            if num_used_paths > 0:
                # Number of flows splits is always one less than the number of
                # paths
                total_flow_splits += (num_used_paths - 1)

        # The max_flow_splits value is incremented by 1 when calculating the
        # fractional component to avoid the instance where the total_flow_splits
        # is equal to the max_flow_splits which would make the fractional_component
        # equal to 1 thus incrementing the integral part of the metric which is
        # incorrect.
        fractional_component = total_flow_splits / (max_flow_splits + 1)
        assert fractional_component < 1, \
            'The fractional component should not be greater than 1'

        metric_value = num_flows_with_split + fractional_component
        return metric_value

    def _calculate_delay_distribution_metric(self, chromosome):
        """Calculates the delay distribution metric for a given chromosome.

        The delay distribution metric is a measure of how the data rate is
        distributed among the given set of paths. The more data rate on the
        lower delay paths, the better. The maximum value the metric can obtain
        for a single flow is 1. A value of 1 means that the flow is being
        transmitted solely on the path with the lowest delay value available.
        The metric is calculated as follows on a per flow basis: Get the paths
        and the data rate allocated on that path and generate a multiplier for
        each path that has some data rate transmitted on it. The multiplier for
        a particular path is generated using the following formula:

            1 / (difference from smallest delay value) + 1

        The data rate transmitted on the given path is multiplied with the path
        multiplier. This is repeated for all paths and the results summed. Finally,
        the result is divided by the total allocated data rate such that the metric
        value for each flow can only have a range of 0 - 1. This is required so as
        not to let the metric be affected by the data rate currently being
        transmitted.

        :param chromosome: The chromosome to be evaluated
        :return: The delay distribution metric.
        """
        metric_value = 0

        for flow in self.flows.values():  # Loop through all the flows
            # Path Delay value: Data Rate passing through that path
            path_delay_data = {flow.get_path_cost(path_id): chromosome[path_id]
                               for path_id in flow.get_path_ids()
                               if chromosome[path_id] > 0}

            if not path_delay_data:  # path_delay_data is empty
                continue

            lowest_delay_path = min([flow.get_path_cost(path_id)
                                     for path_id in flow.get_path_ids()])

            # Calculate the delay distribution metric for the current flow
            flow_metric_value = 0
            for path_delay, data_rate in path_delay_data.items():
                path_multiplier = 1 / ((path_delay - lowest_delay_path) + 1)
                flow_metric_value += (path_multiplier * data_rate)

            # Normalise the flow metric value by the allocated data rate
            flow_metric_value /= sum(path_delay_data.values())
            assert flow_metric_value <= 1, 'The flow delay distribution metric can never exceed 1'
            metric_value += flow_metric_value

        return metric_value

    def _calculate_path_standard_deviation_metric(self, chromosome) -> float:
        """ Calculate the path cost standard deviation for a given chromosome.

        The path standard deviation metric is a measure of the delay difference
        between the paths that a flow is transmitting on.  This metric is
        useful, because if we are transmitting on paths with a very large delay
        difference, then this will have a negative impact on the delay and
        MSTCP receiver buffer size.  Therefore, the aim of this metric is to
        minimise the variation (hence, why we are using the standard deviation)
        in the cost (delay) of the paths used for transmission to minimise the
        negative impact on delay and MSTCP receiver buffer size.

        :param chromosome: The chromosome to be evaluated
        :return: The path standard deviation metric.
        """
        metric_value = 0.0

        self.log_info('_calculate_path_standard_deviation_metric - '
                      '(Calculating the path standard deviation metric for chromosome: {}'
                      .format(chromosome))

        for flow in self.flows.values():
            # Get the list of paths that are being used/allocated any data rate
            used_paths = [path_id for path_id in flow.get_path_ids() if chromosome[path_id] > 0]

            # Get the cost of the used paths
            path_costs = [flow.paths[path_id].cost for path_id in used_paths]

            # Calculate the path cost standard deviation. If only one path is
            # used the standard deviation is equal to 0.
            flow_path_std_dev = statistics.pstdev(path_costs) if len(path_costs) > 1 else 0.0

            metric_value += flow_path_std_dev

            self.log_info('_calculate_path_standard_deviation_metric - '
                          'Flow: {} | Used Paths: {} | Path Costs: {} | Flow Path Std Dev: {} | Objective Value: {}'
                          .format(flow.id, used_paths, path_costs, flow_path_std_dev, metric_value))

        return metric_value

    def _calculate_max_delay_metric(self, chromosome) -> float:
        """Calculate the Maximum Delay metric for a given chromosome.

        The metric value for an individual flow is calculated by multiplying the
        largest path delay from the set of used paths, with the fraction of data
        rate the flow is allocated compared to the total network flow. The final
        metric value is created by adding all the individual flow metrics
        together.

        In this metric, the largest path delay is taken to be the flow's average
        delay. Although simple, this metric is a good representative of the
        actual performance when compared to the results achieved when simulating
        a queue model.

        Args:
            chromosome: The chromosome to be evaluated.

        Returns:
            float: The Maximum Delay metric
        """
        total_network_flow = sum(chromosome)

        self.log_info('_calculate_max_delay_metric - Calculating the maximum delay metric for '
                      F'chromosome: {chromosome}')

        metric_value = 0.0

        for flow in self.flows.values():
            # Get the list of paths that are being used/allocated any data rate
            used_paths = [path_id for path_id in flow.get_path_ids() if chromosome[path_id] > 0]

            if len(used_paths) > 0:  # The metric is valid only if a flow is assigned any data
                # Find the path with the largest cost
                largest_path_cost = max([flow.paths[path_id].cost for path_id in used_paths])
                allocated_data_rate = sum([chromosome[path_id] for path_id in used_paths])
                flow_metric_value = (allocated_data_rate / total_network_flow) * largest_path_cost

                self.log_info('_calculate_max_delay_metric - '
                              F'Flow: {flow.id} | Used Paths: {used_paths} | '
                              F'Largest path cost: {largest_path_cost} | '
                              F'Allocated Data Rate: {allocated_data_rate} | '
                              F'Total Network Flow: {total_network_flow} | '
                              F'Flow Metric: {flow_metric_value}')

                metric_value += flow_metric_value

        return metric_value

    @staticmethod
    def _normalise_value(value, max_value):
        """Normalise the value to have a range between 0 and 1.

        Checks are made to ensure that the normalised value is between 0 and 1.

        :param value:     The value to normalise.
        :param max_value: The largest amount the value can take. This parameter
                          is used to normalise the value variable with.

        :return: The normalised value.
        """

        # If both the maximum value, and the value itself are zero, then return zero
        if math.isclose(value, 0, abs_tol=ACCURACY_VALUE) and math.isclose(max_value, 0, abs_tol=ACCURACY_VALUE):
            return 0.0

        normalised_value = value / max_value

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
