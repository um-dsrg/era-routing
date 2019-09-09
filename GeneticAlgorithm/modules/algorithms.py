"""Contains various different evolutionary algorithms"""

import random
from typing import Dict

from lxml import etree

from deap import algorithms, tools
from modules.timings import GaTimings


def nsga2(parameters, logger, ga_operators, ga_stats, ga_results, result_xml, toolbox):
    """Implements the NSGA-II algorithm.

    Run the NSGA-II algorithm for the specified number of generations and store
    the results in an XML file.
    """
    ga_timing = GaTimings(parameters.num_generations, logger.log_status)
    ga_timing.log_duration_start()  # Log the start time

    # Generate the first population
    population = toolbox.population(n=parameters.pop_size)
    population = ga_operators.round_small_numbers(population)

    # If the PcLp result file is given add it to the first population
    if parameters.pclpResultFile:
        logger.log_info(F"Adding the PcLp result to population. File: {parameters.pclpResultFile}")
        xmlParser = etree.XMLParser(remove_blank_text=True)
        resFileRoot = etree.parse(parameters.pclpResultFile, xmlParser).getroot()

        pathAssignment = dict()  # type: Dict[int, float]

        for pathElement in resFileRoot.findall("OptimalSolution/Flow/Path"):
            pathId = int(pathElement.get("Id"))
            pathDataRate = float(pathElement.get("DataRate"))

            if pathId in pathAssignment:
                raise RuntimeError("Trying to insert duplicate path when generating lp solution. "
                                   F"Path Id: {pathId}")

            pathAssignment[pathId] = pathDataRate

        chromosomeIndex = random.randint(0, len(population) - 1)
        chromosomeToChange = population[chromosomeIndex]

        for pathId, pathDataRate in sorted(pathAssignment.items()):
            logger.log_info(F"Path ID: {pathId} | Data Rate: {pathDataRate}")
            chromosomeToChange[pathId] = pathDataRate

        population[chromosomeIndex] = chromosomeToChange

    logger.log_info(F"The chromosome is {population[chromosomeIndex]}")

    population = ga_stats.reset_chromosome_counters(population)

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
        logger.log_info("Starting generation {}".format(gen))
        logger.log_status('Starting generation {}'.format(gen))

        ga_stats.set_generation(gen)
        ga_timing.log_generation_start()

        # Select individuals using tournament selection based on dominance and
        # crowding distance that will be used for mating and mutation
        # operations.
        offspring = tools.selTournamentDCD(population, parameters.pop_size)

        # Apply crossover and mutation to the offspring population
        offspring = algorithms.varAnd(offspring, toolbox, parameters.prob_crossover,
                                      parameters.prob_mutation)

        offspring = ga_operators.round_small_numbers(offspring)

        # Evaluate individuals with an invalid fitness
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
        for ind, fit in zip(invalid_ind, fitness_values):
            ind.fitness.values = fit

        if parameters.saveParentOffspring:
            logger.log_info("Saving the Parent + Offspring population in the result file")
            ga_results.add_combined_population(gen, population, offspring)

        # Select the best individuals from the current population and offspring
        population = tools.selNSGA2(population + offspring, parameters.pop_size)

        # Log the chromosomes that were generated via crossover/mutation that
        # survived to the next generation
        ga_stats.log_survivors(population)

        # Reset the population chromosome counters
        population = ga_stats.reset_chromosome_counters(population)

        # Log the current generation duration
        ga_timing.log_generation_end(gen)

        # Add the current population to the results
        ga_results.add_population(gen, population)

        if gen % parameters.xml_save_frequency == 0:  # Append results to the result file
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


def nsga3(parameters, logger, ga_operators, ga_stats, ga_results, result_xml, objectives, toolbox):
    """Implements the NSGA-III algorithm.

    Run the NSGA-III algorithm for the specified number of generations and store
    the results in an XML file.
    """
    ga_timing = GaTimings(parameters.num_generations, logger.log_status)
    ga_timing.log_duration_start()  # Log the start time

    # Generate the first population
    population = toolbox.population(n=parameters.pop_size)
    population = ga_operators.round_small_numbers(population)
    population = ga_stats.reset_chromosome_counters(population)

    # Evaluate individuals with an invalid fitness
    invalid_ind = [ind for ind in population if not ind.fitness.valid]
    fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
    for ind, fit in zip(invalid_ind, fitness_values):
        ind.fitness.values = fit

    # Store the initial population before evolution starts.
    ga_results.add_population(0, population)

    # Generate the reference points
    reference_points = tools.uniform_reference_points(objectives.gen_num_objectives(),
                                                      p=parameters.nsga3_p)

    if len(reference_points) > parameters.pop_size:
        raise AssertionError("The number of reference points is larger than the population size. "
                             "Reference Points: {} | Population Size: {}"
                             .format(len(reference_points), parameters.pop_size))

    logger.log_info("Number of reference points to be used: {}".format(len(reference_points)))
    ga_stats.log_nsga3_reference_points(reference_points)
    nsga3_selector = tools.selNSGA3WithMemory(reference_points, nd="standard")

    # Start the evolution process
    for gen in range(1, parameters.num_generations + 1):
        logger.log_info("Starting generation {}".format(gen))
        logger.log_status('Starting generation {}'.format(gen))

        ga_stats.set_generation(gen)
        ga_timing.log_generation_start()

        offspring = tools.selRandom(population, parameters.pop_size)

        # Apply crossover and mutation to the offspring population
        offspring = algorithms.varAnd(offspring, toolbox, parameters.prob_crossover,
                                      parameters.prob_mutation)

        offspring = ga_operators.round_small_numbers(offspring)

        # Evaluate individuals with an invalid fitness
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
        for ind, fit in zip(invalid_ind, fitness_values):
            ind.fitness.values = fit

        if parameters.saveParentOffspring:
            logger.log_info("Saving the Parent + Offspring population in the result file")
            ga_results.add_combined_population(gen, population, offspring)

        # Select the best individuals from the current population and offspring
        population = nsga3_selector(population + offspring, parameters.pop_size)

        # Log the chromosomes that were generated via crossover/mutation that
        # survived to the next generation
        ga_stats.log_survivors(population)

        # Reset the population chromosome counters
        population = ga_stats.reset_chromosome_counters(population)

        # Log the current generation duration
        ga_timing.log_generation_end(gen)

        # Add the current population to the results
        ga_results.add_population(gen, population)

        if gen % parameters.xml_save_frequency == 0:  # Append results to the result file
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
