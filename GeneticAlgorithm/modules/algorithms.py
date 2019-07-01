"""Contains various different evolutionary algorithms"""

from deap import algorithms, tools
from modules.timings import GaTimings


def spea2(parameters, logger, ga_operators, ga_stats, ga_results, result_xml, toolbox):
    """Implements the SPEA2 algorithm.

    """
    raise NotImplementedError("The SPEA2 algorithm is not implemented yet")

    # ga_timing = GaTimings(parameters.num_generations, logger.log_status)
    # ga_timing.log_duration_start()  # Log the start time

    # # Generate the first population
    # population = toolbox.population(n=parameters.pop_size)
    # population = ga_operators.round_small_numbers(population)
    # population = ga_stats.reset_chromosome_counters(population)

    # # Evaluate individuals with an invalid fitness
    # invalid_ind = [ind for ind in population if not ind.fitness.valid]
    # fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
    # for ind, fit in zip(invalid_ind, fitness_values):
    #     ind.fitness.values = fit

    # # Assign a crowding distance to the individuals. No selection is actually
    # # done in this step
    # population = tools.selNSGA2(population, parameters.pop_size)

    # # Store the initial population before evolution starts.
    # ga_results.add_population(0, population)

    # # Start the evolution process
    # for gen in range(1, parameters.num_generations + 1):
    #     logger.log_info("Starting generation {}".format(gen))
    #     logger.log_status('Starting generation {}'.format(gen))

    #     ga_stats.set_generation(gen)
    #     ga_timing.log_generation_start()

    #     # Select individuals using tournament selection based on dominance and
    #     # crowding distance that will be used for mating and mutation
    #     # operations.
    #     offspring = tools.selTournamentDCD(population, parameters.pop_size)

    #     # Create a deep copy of the offspring
    #     offspring = [toolbox.clone(ind) for ind in offspring]

    #     # Apply crossover and mutation to the offspring population
    #     offspring = algorithms.varAnd(offspring, toolbox, parameters.prob_crossover,
    #                                   parameters.prob_mutation)

    #     offspring = ga_operators.round_small_numbers(offspring)

    #     # Evaluate individuals with an invalid fitness
    #     invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
    #     fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
    #     for ind, fit in zip(invalid_ind, fitness_values):
    #         ind.fitness.values = fit

    #     # Select the best individuals from the current population and offspring
    #     population = tools.selNSGA2(population + offspring, parameters.pop_size)

    #     # Log the chromosomes that were generated via crossover/mutation that
    #     # survived to the next generation
    #     ga_stats.log_survivors(population)

    #     # Reset the population chromosome counters
    #     population = ga_stats.reset_chromosome_counters(population)

    #     # Log the current generation duration
    #     ga_timing.log_generation_end(gen)

    #     # Add the current population to the results
    #     ga_results.add_population(gen, population)

    #     if gen % parameters.xml_save_frequency == 0:  # Append results to the result file
    #         logger.log_status('Appending the results. Generation: {}'.format(gen))
    #         ga_results.append_to_xml(result_xml.get_root())
    #         ga_stats.append_to_xml(result_xml.get_root())
    #         result_xml.save_xml_file()

    # ga_timing.log_duration_end()

    # # Call append again just in case the number of generations is not exactly
    # # divisible by the frequency parameter set by the user.
    # ga_results.append_to_xml(result_xml.get_root())
    # ga_stats.append_to_xml(result_xml.get_root())
    # ga_timing.add_to_xml(result_xml.get_root())
    # result_xml.save_xml_file()


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

        # Create a deep copy of the offspring
        offspring = [toolbox.clone(ind) for ind in offspring]

        # Apply crossover and mutation to the offspring population
        offspring = algorithms.varAnd(offspring, toolbox, parameters.prob_crossover,
                                      parameters.prob_mutation)

        offspring = ga_operators.round_small_numbers(offspring)

        # Evaluate individuals with an invalid fitness
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
        for ind, fit in zip(invalid_ind, fitness_values):
            ind.fitness.values = fit

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
    logger.log_info("Number of reference points to be used: {}".format(len(reference_points)))

    if len(reference_points) > parameters.pop_size:
        raise AssertionError("The number of reference points is larger than the population size. "
                             "Reference Points: {} | Population Size: {}"
                             .format(len(reference_points), parameters.pop_size))

    nsga3_selector = tools.selNSGA3WithMemory(reference_points)

    # Start the evolution process
    for gen in range(1, parameters.num_generations + 1):
        logger.log_info("Starting generation {}".format(gen))
        logger.log_status('Starting generation {}'.format(gen))

        ga_stats.set_generation(gen)
        ga_timing.log_generation_start()

        # FIXME: Update the below to use random selection
        # offspring = tools.selTournamentDCD(population, parameters.pop_size)
        # # Create a deep copy of the offspring
        # offspring = [toolbox.clone(ind) for ind in offspring]

        # Apply crossover and mutation to the offspring population
        offspring = algorithms.varAnd(population, toolbox, parameters.prob_crossover,
                                      parameters.prob_mutation)

        offspring = ga_operators.round_small_numbers(offspring)

        # Evaluate individuals with an invalid fitness
        invalid_ind = [ind for ind in offspring if not ind.fitness.valid]
        fitness_values = toolbox.map(toolbox.evaluate, invalid_ind)
        for ind, fit in zip(invalid_ind, fitness_values):
            ind.fitness.values = fit

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
