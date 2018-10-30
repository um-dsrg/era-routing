from collections import OrderedDict
from statistics import mean
from timeit import default_timer as timer

from lxml import etree


class GaTimings:
    """Store various timers in relation to the Genetic Algorithm.

    Attributes
        duration_start_time: Logs the start time. This variable will time the
                             duration of the whole algorithm.
        duration_time:       The total time, in seconds, the Genetic algorithm
                             took to run.
        gen_start_time:      Logs the start time of the current generation.
                             This variable will time the duration of a single
                             generation.
        gen_duration:        The total time, in seconds, each generation took.
                             Key: Gen number | Value: Duration in secs.
    """

    def __init__(self, num_generations, log_status):
        self.duration_start_time = 0.0
        self.duration_time = 0.0
        self.gen_start_time = 0.0
        self.gen_duration = OrderedDict()
        self.num_generations = num_generations
        self.log_status = log_status

    def log_duration_start(self):
        self.duration_start_time = timer()

    def log_duration_end(self):
        self.duration_time = timer() - self.duration_start_time

    def log_generation_start(self):
        self.gen_start_time = timer()

    def log_generation_end(self, gen_num):
        gen_end_time = timer()
        self.gen_duration[gen_num] = gen_end_time - self.gen_start_time

        if len(self.gen_duration) >= 5:
            prev_timings_avg = mean([self.gen_duration[gen]
                                     for gen
                                     in list(self.gen_duration.keys())[-5:]])
            remaining_gens = self.num_generations - gen_num
            estimated_time = prev_timings_avg * remaining_gens
            self.log_status('Estimated time remaining: {}'
                            .format(secs_to_str(estimated_time)))

    def add_to_xml(self, xml_root):
        """Add the timing results to the XML file.

        :param xml_root: The root of the result XML file.
        """
        xml_element = etree.SubElement(xml_root, 'Timings')
        xml_element.append(etree.Comment('All timings are in Seconds'))

        duration_element = etree.SubElement(xml_element, 'Duration')
        duration_element.text = str(self.duration_time)

        generations_element = etree.SubElement(xml_element, 'Generations')
        for gen_num, gen_duration in sorted(self.gen_duration.items()):
            gen_element = etree.SubElement(generations_element, 'Generation')
            gen_element.set('Number', str(gen_num))
            gen_element.text = str(gen_duration)


def secs_to_str(seconds: float):
    """Converts seconds to days:hours:minutes:seconds format."""
    minutes, seconds = divmod(seconds, 60)
    hours, minutes = divmod(minutes, 60)
    days, hours = divmod(hours, 24)

    return ('{:02g} day(s) {:02g} hour(s) {:02g} minute(s) '
            '{:2.2f} second(s)'.format(days, hours, minutes, seconds))
