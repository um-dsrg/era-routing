#!/usr/bin/env python3
"""
An implementation of a multi-objective genetic algorithm using NSGA-II to find
the MaxFlowMinCostMinPaths of a network.

A Chromosome is a list of values that represent the amount of flow that is
travelling through that path. For example, the chromosome [1, 4, 0, 5] means
that 1 is transmitted through path 0, 4 through path 1, 0 through path 2 and 5
through path 3.

Which links the paths used are determined by the network matrix. The network
matrix is read from an XML path that contain the k shortest paths from each
source to destination. More information about the network matrix can be found
in the ga_helper::Network class.
"""
from deap import algorithms, base, creator, tools

from modules.flow import Flow
from modules.ga_operators import GaOperators
from modules.ga_results import GaResults
from modules.ga_statistics import GaStatistics
from modules.logger import Logger
from modules.network import Network
from modules.objectives import Objectives
from modules.parameters import Parameters
from modules.timings import GaTimings
from modules.xml_handler import XmlHandler


def run_nsga2_ga(parameters, logger, ga_stats, ga_results, result_xml, toolbox):
    """Run the NSGA-II algorithm.

    Run the NSGA-II algorithm for the specified number of generations and store
    the results in an XML file.
    """
    ga_timing = GaTimings(parameters.num_generations, logger.log_status)
    ga_timing.log_duration_start()  # Log the start time

    # Generate the first population
    population = toolbox.population(n=parameters.pop_size)

    # Evaluate individuals with an invalid fitness
    invalid_ind = [ind for ind in population if not ind.fitness.valid]
    fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
    for ind, fit in zip(invalid_ind, fitness_values):
        ind.fitness.values = fit

    # Assign a crowding distance to the individuals. No selection is actually
    # done in this step
    population = tools.selNSGA2(population, parameters.pop_size)

    # Store the initial population before evolution starts.
    ga_results.add_population(0, population)

    # Start the evolution process
    for gen in range(1, parameters.num_generations + 1):
        logger.log_status('Starting generation {}'.format(gen))
        ga_stats.set_generation(gen)
        ga_timing.log_generation_start()

        # Select individuals using tournament selection based on dominance and
        # crowding distance that will be used for mating and mutation
        # operations.
        offspring = tools.selTournamentDCD(population, parameters.pop_size)

        # Create a deep copy of the offspring
        offspring = [toolbox.clone(ind) for ind in offspring]

        # Apply crossover and mutation to the offspring population
        offspring = algorithms.varAnd(offspring, toolbox, parameters.prob_crossover, parameters.prob_mutation)

        # Store which mutation operations were carried out
        off_mut_type_counter = ga_stats.count_mutation_operations(offspring)

        # Evaluate individuals with an invalid fitness
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
        for ind, fit in zip(invalid_ind, fitness_values):
            ind.fitness.values = fit

        # Reset the population's mutation operators
        ga_stats.reset_mutation_operations(population)

        # Select the best individuals from the current population and offspring
        population = tools.selNSGA2(population + offspring, parameters.pop_size)

        # Log the mutation operations that survived after selection
        ga_stats.log_survived_mutation(population, off_mut_type_counter)

        # Log the current generation duration
        ga_timing.log_generation_end(gen)

        # Add the current population to the results
        ga_results.add_population(gen, population)

        if gen % parameters.xml_save_frequency == 0:
            # Append the current results to the XML result file
            logger.log_status('Appending the results. Generation: {}'.format(gen))
            ga_results.append_to_xml(result_xml.get_root())
            ga_stats.append_to_xml(result_xml.get_root())
            result_xml.save_xml_file()

    ga_timing.log_duration_end()

    # Call append again just in case the number of generations is not exactly
    # divisible by the frequency parameter set by the user.
    ga_results.append_to_xml(result_xml.get_root())
    ga_stats.append_to_xml(result_xml.get_root())
    ga_timing.add_to_xml(result_xml.get_root())
    result_xml.save_xml_file()


def main():
    """Main function that sets up and runs the Genetic Algorithm."""
    parameters = Parameters()
    logger = Logger(parameters)
    objectives = Objectives(parameters.objectives)
    ksp_xml = XmlHandler(parameters.ksp_xml_file)
    flows = Flow.parse_flows(ksp_xml.get_root())
    network = Network(ksp_xml.get_root(), flows, objectives, logger.log_info)

    ga_stats = GaStatistics(parameters.num_generations)
    ga_operators = GaOperators(flows, network, parameters, objectives, ga_stats, logger.log_info)

    # # # Configure the GA objectives # # #
    creator.create('MaxFlowMinCost', base.Fitness, weights=objectives.get_obj_weights())
    creator.create('Chromosome', list, fitness=creator.MaxFlowMinCost)

    # # # Configure the GA operators # # #
    toolbox = base.Toolbox()
    toolbox.register('indices', ga_operators.generate_chromosome)
    toolbox.register('individual', tools.initIterate, creator.Chromosome, toolbox.indices)
    toolbox.register('population', tools.initRepeat, list, toolbox.individual)
    toolbox.register('evaluate', ga_operators.evaluate_chromosome)
    toolbox.register('mate', ga_operators.mate_chromosomes)
    toolbox.register('mutate', ga_operators.mutate_chromosome)

    # # # Store the configuration parameters # # #
    logger.log_status('Saving Configuration parameters in XML file')
    result_xml = XmlHandler(parameters.result_file, 'GeneticAlgorithm')
    parameters.append_to_xml(result_xml.get_root())
    network.append_to_xml(result_xml.get_root())
    objectives.append_to_xml(result_xml.get_root())

    # # # Run the NSGA-II Algorithm # # #
    ga_results = GaResults(parameters, objectives)
    run_nsga2_ga(parameters, logger, ga_stats, ga_results, result_xml, toolbox)

    logger.log_status('Simulation complete.')


if __name__ == "__main__":
    main()
