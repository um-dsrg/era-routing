"""Contains the GaResults class"""
from lxml import etree


class GaResults:
    """Class that stores the Genetic Algorithm's results.

    Attributes

    population_by_generation: A dictionary that will store the results per
                              generation. The Generation Number is set as Key
                              and the population (list of chromosomes) as the
                              Value.
    """

    def __init__(self, parameters, objectives):
        """Initialise a GaResults object to allow storing the results per
        generation.

        :param parameters: The Parameters object used to access the genetic
                           algorithm configuration parameters.
        :param objectives: Instance of the Objectives class.
        """
        self.store_all_genes = parameters.store_genes
        self.num_generations = parameters.num_generations
        self.population_by_generation = dict()
        self.parent_population_by_gen = dict()
        self.offspring_population_by_gen = dict()
        self.population_xml_element = None
        self.combined_population_xml_element = None
        self.obj_names = objectives.get_obj_names()

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

    def add_combined_population(self, n_generation: int,
                                parent_pop: list, offspring_pop: list) -> None:
        """Add the parent and offspring population to the result set

        Arguments:
            n_generation {int} -- The generation at which this population has
                                  been generated
            parent_pop {list} -- The parent population
            offspring_pop {list} -- The offspring population

        Raises:
            RuntimeError: Raised when trying to insert a duplicate population
        """
        if (n_generation in self.parent_population_by_gen or
                n_generation in self.offspring_population_by_gen):
            raise RuntimeError(f"Inserting duplicate generation in combined population results. "
                               "Generation number: {n_generation}")
        else:
            self.parent_population_by_gen[n_generation] = parent_pop
            self.offspring_population_by_gen[n_generation] = offspring_pop

    def append_to_xml(self, xml_root):
        """Append the results to the xml result file.

        Append the generated results to the XML result file. The results that
        are stored in the XML file are deleted from memory.

        :param xml_root: The root of the XML result file.
        """
        # # # Population # # #
        if self.population_xml_element is None:
            self.population_xml_element = etree.SubElement(xml_root, 'Population')

        for gen, population in sorted(self.population_by_generation.items()):
            gen_element = etree.Element('Generation')
            gen_element.set('number', str(gen))

            for chromosome in population:
                chromo_element = etree.SubElement(gen_element, 'Chromosome')
                for obj_name, obj_value in zip(self.obj_names,
                                               chromosome.fitness.values):
                    chromo_element.set(obj_name, str(obj_value))

                # Store the genes only in the last population OR if the
                # store_all_genes flag is set
                if self.store_all_genes is True or gen == self.num_generations:
                    for idx, gene in enumerate(chromosome):
                        gene_element = etree.SubElement(chromo_element, 'Gene')
                        gene_element.set('path_id', str(idx))
                        gene_element.set('value', str(gene))

            self.population_xml_element.append(gen_element)
            self.population_by_generation = dict()  # Clear the dictionary

        # # # Combined population # # #
        if not (not self.parent_population_by_gen and not self.offspring_population_by_gen):

            # Create the XML element
            if self.combined_population_xml_element is None:
                self.combined_population_xml_element = etree.SubElement(xml_root,
                                                                        "CombinedPopulation")

            for gen_number in sorted(self.parent_population_by_gen.keys()):
                gen_element = etree.SubElement(self.combined_population_xml_element, "Generation")
                gen_element.set("number", str(gen_number))

                # # # Save the parent population # # #
                parent_population = self.parent_population_by_gen[gen_number]
                parent_pop_element = etree.SubElement(gen_element, "Parent")

                for chromosome in parent_population:
                    chromo_element = etree.SubElement(parent_pop_element, "Chromosome")
                    for obj_name, obj_value in zip(self.obj_names, chromosome.fitness.values):
                        chromo_element.set(obj_name, str(obj_value))

                gen_element.append(parent_pop_element)

                # # # Save the offspring population # # #
                offspring_population = self.offspring_population_by_gen[gen_number]
                offspring_pop_element = etree.SubElement(gen_element, "Offspring")

                for chromosome in offspring_population:
                    chromo_element = etree.SubElement(offspring_pop_element, "Chromosome")
                    for obj_name, obj_value in zip(self.obj_names, chromosome.fitness.values):
                        chromo_element.set(obj_name, str(obj_value))

                gen_element.append(offspring_pop_element)

                self.combined_population_xml_element.append(gen_element)

            # Clear the dictionaries after saving them in result file
            self.parent_population_by_gen = dict()
            self.offspring_population_by_gen = dict()
