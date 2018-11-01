from lxml import etree

from .parameters import Parameters


class GaResults:
    """Class that stores the Genetic Algorithm's results.

    Attributes

    population_by_generation: A dictionary that will store the results per
                              generation. The Generation Number is set as Key
                              and the population (list of chromosomes) as the
                              Value.
    """
    def __init__(self, parameters: Parameters):
        """Initialise a GaResults object to allow storing the results per
        generation.

        :param parameters: The Parameters object used to access the genetic
                           algorithm configuration parameters.
        """
        self.store_all_genes = parameters.store_genes
        self.num_generations = parameters.num_generations
        self.population_by_generation = dict()
        self.xml_element = None

    def add_population(self, n_generation: int, population):
        """Add the population to the result set.

        :param n_generation: The current generation number.
        :param population:   The population.
        """
        if n_generation in self.population_by_generation:
            raise RuntimeError('Inserting duplicate generation in results.'
                               'Generation number: {}'.format(n_generation))
        else:
            self.population_by_generation[n_generation] = population

    def append_to_xml(self, xml_root):
        """Append the results to the xml result file.

        Append the generated results to the XML result file. The results that
        are stored in the XML file are deleted from memory.

        :param xml_root: The root of the XML result file.
        """
        if self.xml_element is None:
            self.xml_element = etree.SubElement(xml_root, 'Population')

        for gen, population in sorted(self.population_by_generation.items()):
            gen_element = etree.Element('Generation')
            gen_element.set('number', str(gen))

            for chromosome in population:
                chromo_element = etree.SubElement(gen_element, 'Chromosome')
                chromo_element.set('net_flow',
                                   str(chromosome.fitness.values[0]))
                chromo_element.set('net_cost',
                                   str(chromosome.fitness.values[1]))
                chromo_element.set('paths_used',
                                   str(chromosome.fitness.values[2]))

                # Store the genes only in the last population OR if the
                # store_all_genes flag is set
                if self.store_all_genes is True or gen == self.num_generations:
                    for idx, gene in enumerate(chromosome):
                        gene_element = etree.SubElement(chromo_element, 'Gene')
                        gene_element.set('path_id', str(idx))
                        gene_element.set('value', str(gene))

            self.xml_element.append(gen_element)
            self.population_by_generation = dict()  # Clear the dictionary
