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
from deap import base, creator, tools

from modules.flow import Flow
from modules.ga_operators import GaOperators
from modules.ga_results import GaResults
from modules.ga_statistics import GaStatistics
from modules.logger import Logger
from modules.network import Network
from modules.objectives import Objectives
from modules.parameters import Parameters
from modules.xml_handler import XmlHandler
from modules.algorithms import spea2, nsga2, nsga3


def main():
    """Main function that sets up and runs the Genetic Algorithm."""
    parameters = Parameters()
    logger = Logger(parameters)
    objectives = Objectives(parameters.objectives)
    inputXml = XmlHandler(parameters.inputFile)
    flows = Flow.parse_flows(inputXml.get_root())
    network = Network(inputXml.get_root(), flows, objectives, logger.log_info)

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
    resultXml = XmlHandler(parameters.outputFile, 'GeneticAlgorithm')
    parameters.append_to_xml(resultXml.get_root())
    network.append_to_xml(resultXml.get_root())
    objectives.append_to_xml(resultXml.get_root())

    # # # Initialise the results container # # #
    ga_results = GaResults(parameters, objectives)

    # # # Start the evolution process # # #
    if parameters.algorithm == "spea2":
        logger.log_status("Starting the evolution using the SPEA 2 algorithm")
        spea2(parameters, logger, ga_operators, ga_stats, ga_results, resultXml, toolbox)
    elif parameters.algorithm == "nsga2":
        logger.log_status("Starting the evolution using the NSGA-II algorithm")
        nsga2(parameters, logger, ga_operators, ga_stats, ga_results, resultXml, toolbox)
    elif parameters.algorithm == "nsga3":
        logger.log_status("Starting the evolution using the NSGA-III algorithm")
        nsga3(parameters, logger, ga_operators, ga_stats, ga_results, resultXml, toolbox)
    else:
        raise AssertionError("Unknown algorithm given. {}".format(parameters.algorithm))

    logger.log_status("Evolution complete")


if __name__ == "__main__":
    main()
