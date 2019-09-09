import argparse
import os

from lxml import etree


class Parameters:
    def __init__(self):
        self._parse_cmd_line_args()

    def append_to_xml(self, xml_root_element):
        """Append the configuration parameters to the XML result file."""
        params_element = etree.SubElement(xml_root_element, 'Parameters')
        params_element.set('num_generations', str(self.num_generations))
        params_element.set('pop_size', str(self.pop_size))
        params_element.set('prob_crossover', str(self.prob_crossover))
        params_element.set('p_mutation', str(self.prob_mutation))
        params_element.set('mutation_fraction', str(self.mutation_fraction))

    @staticmethod
    def _set_cmd_line_args():
        """Set the command line parser."""
        parser = argparse.ArgumentParser()
        parser.add_argument("-i", "--input", required=True,
                            help='The location to the path selection result file')
        parser.add_argument("-o", "--output", required=True,
                            help='The location where to save the XML result file.')
        parser.add_argument("--pclpResultFile", required=False, type=str, default="",
                            help="The path to the pclp result file")
        parser.add_argument("--algorithm", type=str, required=True,
                            help="The Evolutionary algorithm to run. Available options are: "
                                 "nsga2 | nsga3")
        parser.add_argument('--num_generations', type=int, required=True,
                            help='The number of generations the Genetic '
                                 'Algorithm will loop for.')
        parser.add_argument('--pop_size', type=int, required=True,
                            help='The population size.')
        parser.add_argument('--prob_crossover', type=float, required=True,
                            help='Crossover probability.')
        parser.add_argument('--prob_mutation', type=float, required=True,
                            help='Mutation probability.')
        parser.add_argument('--mutation_fraction', type=float, required=True,
                            help='The fraction of the chromosome size that '
                                 'will be mutated. This variable dictates the '
                                 'number of genes that will be modified by a '
                                 'single mutation operator.')
        parser.add_argument('--mutationFunctions', type=str, required=True, default="",
                            help="A comma separated list of the functions to be used when carrying "
                                 "out a mutation operation")
        parser.add_argument('--mutationFunctionProbability', type=str, required=True, default="",
                            help="A comma separated list of the probability to associate with each "
                                 "mutation function. Note that the sum of the values must always "
                                 "add up to 1")
        parser.add_argument("--nsga3_p", type=int, required=False, default=0,
                            help="The p value that will be used to calculate the number of "
                                 "reference points to generate")
        parser.add_argument('--objectives', required=True, nargs='*', type=str,
                            help='List of objectives the Genetic Algorithm will '
                                 'work on. The objectives need to be given in '
                                 'this order: '
                                 'name, weight (maximise=1 / minimise=-1), '
                                 'metric calculation function, bound calculation function')
        parser.add_argument('--log_directory', type=str, required=False,
                            help='The path where to store the log files.')
        parser.add_argument('--status_log', action='store_true',
                            required=False,
                            help='When set, the status log will be generated.')
        parser.add_argument('--info_log', action='store_true', required=False,
                            help='When set, the information log will be '
                                 'generated.')
        parser.add_argument('--xml_save_frequency', type=int, required=False,
                            help='The frequency, in generations, of how often '
                                 'to update the XML result file with new '
                                 'results. If not given, the value is set '
                                 'equal to the number of generations.')
        parser.add_argument('--store_genes', required=False,
                            action='store_true',
                            help='When enabled the result file will store the '
                                 'genes for all the Chromosomes for all the '
                                 'population for all the generations. When '
                                 'this flag is disabled the genes; will only '
                                 'be stored for the final population.')
        parser.add_argument("--saveParentOffspring", required=False, action="store_true",
                            help="When set, both the parent and offspring population will be added "
                                 "to the result file")

        parser.set_defaults(status_log=False)
        parser.set_defaults(info_log=False)
        parser.set_defaults(store_genes=False)
        parser.set_defaults(saveParentOffspring=False)

        return parser.parse_args()

    def _parse_cmd_line_args(self):
        """Parse the command line arguments and check that they are valid."""
        cmd_line_parser = self._set_cmd_line_args()

        # Verify that the file exists
        if os.path.isfile(cmd_line_parser.input) is False:
            raise AssertionError(F"The input file does not exist: {cmd_line_parser.input}")
        self.inputFile = cmd_line_parser.input

        # Verify that the path where the result file will be stored exists
        if os.path.isdir(os.path.dirname(cmd_line_parser.output)) is False:
            raise AssertionError(F"The output directory does not exist: "
                                 F"{os.path.dirname(cmd_line_parser.output)}")
        self.outputFile = cmd_line_parser.output

        # Verify that the pclpResult file exists if given
        if cmd_line_parser.pclpResultFile:
            if os.path.isfile(cmd_line_parser.pclpResultFile) is False:
                raise AssertionError("The pclp result file does not exist: "
                                     F"{cmd_line_parser.pclpResultFile}")
            self.pclpResultFile = cmd_line_parser.pclpResultFile
        else:
            self.pclpResultFile = ""

        # Verify the algorithm is valid
        self.algorithm = cmd_line_parser.algorithm.lower()
        if self.algorithm not in ["nsga2", "nsga3"]:
            raise AssertionError(F"Unknown algorithm given: {self.algorithm}")

        # Ensure that nsga3_p is given when nsga3 is chosen
        if self.algorithm == "nsga3" and cmd_line_parser.nsga3_p <= 0:
            raise AssertionError("nsga_3 p value needs to be provided when using NSGA-III OR "
                                 "invalid nsga_3 p value provided")
        self.nsga3_p = cmd_line_parser.nsga3_p

        # Check the number of generations
        assert cmd_line_parser.num_generations > 0, \
            'Number of generations should be larger than 0'
        self.num_generations = cmd_line_parser.num_generations

        # Check population size
        if cmd_line_parser.pop_size <= 0 or cmd_line_parser.pop_size % 4 != 0:
            raise AssertionError('Population size must be larger than 0 OR '
                                 'Population size needs to be divisible by 4')
        self.pop_size = cmd_line_parser.pop_size

        # Check probability of crossover
        assert 0 <= cmd_line_parser.prob_crossover <= 1, \
            'Crossover probability must be between 0 and 1'
        self.prob_crossover = cmd_line_parser.prob_crossover

        # Check probability of mutation
        assert 0 <= cmd_line_parser.prob_mutation <= 1, \
            'Mutation probability must be between 0 and 1'
        self.prob_mutation = cmd_line_parser.prob_mutation

        # Check mutation fraction
        assert 0 <= cmd_line_parser.mutation_fraction <= 1, \
            'Mutation fraction must be between 0 and 1'
        self.mutation_fraction = cmd_line_parser.mutation_fraction

        # Check mutation functions
        assert cmd_line_parser.mutationFunctions != "", "Mutation functions must be provided"
        self.mutationFunctions = [mutationFunction.strip() for mutationFunction
                                  in cmd_line_parser.mutationFunctions.split(",")]

        # Check mutation function probability
        assert cmd_line_parser.mutationFunctionProbability != "", \
            "Mutation function probability must be provided"
        self.mutationFunctionProbability = \
            [float(mutFuncProb.strip()) for mutFuncProb
             in cmd_line_parser.mutationFunctionProbability.split(",")]

        # Check that mutation functions and the function probabilities given are valid
        assert sum(self.mutationFunctionProbability) == 1, \
            "The mutation function probability MUST add up to 1"

        assert len(self.mutationFunctions) == len(self.mutationFunctionProbability), \
            "The number of mutation functions and probabilities needs to be equal"

        # Check XML save frequency
        if cmd_line_parser.xml_save_frequency is None:
            self.xml_save_frequency = self.num_generations
        else:
            assert 1 <= cmd_line_parser.xml_save_frequency <= self.num_generations, \
                F"The xml_save_frequency should be between 1 and {self.num_generations}"
            self.xml_save_frequency = cmd_line_parser.xml_save_frequency

        # Check to make sure that if logging is enabled, the location where to
        # store the log files is given as well
        if (cmd_line_parser.status_log is True or
                cmd_line_parser.info_log is True) and \
                not cmd_line_parser.log_directory:
            raise AssertionError('Cannot enable logging without giving the '
                                 'location where to store the log files')
        else:
            self.log_directory = cmd_line_parser.log_directory
            self.status_log = cmd_line_parser.status_log
            self.info_log = cmd_line_parser.info_log

        self.store_genes = cmd_line_parser.store_genes
        self.objectives = cmd_line_parser.objectives
        self.saveParentOffspring = cmd_line_parser.saveParentOffspring
