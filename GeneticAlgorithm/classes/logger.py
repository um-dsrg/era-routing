import logging
import logging.handlers

from classes.parameters import Parameters


class Logger:
    """Class that enables logging

    The logger class provides two function pointers the log_status and log_info
    that are used to log the progress of the Genetic Algorithm. By default the
    function pointer point to nothing and are only enabled via command line
    parameters.

    The status logger is used to show the current progress of the algorithm.
    The information logger is used to log every important detail of the
    algorithm and should be only used for debug purposes because a lot of
    information is output by this logger.
    """

    def __init__(self, parameters: Parameters):
        self.log_status = self._logger_disabled
        self.log_info = self._logger_disabled

        if parameters.status_log:  # Status log enabled
            self._setup_status_logger(parameters.log_directory)
            self.log_status = self._log_status

        if parameters.info_log:  # Information log enabled
            self._setup_info_logger(parameters.log_directory)
            self.log_info = self._log_info

    def _setup_status_logger(self, log_directory: str, log_capacity=10):
        """Setup the status logger"""
        handler = logging.FileHandler(log_directory + '/status.log', mode='w')
        formatter = logging.Formatter('%(asctime)s - %(message)s',
                                      '%d-%m-%Y %H:%M:%S')
        handler.setFormatter(formatter)
        mem_handler = logging.handlers.MemoryHandler(log_capacity,
                                                     logging.ERROR,
                                                     handler)

        self._stat_logger = logging.getLogger('status_logger')
        self._stat_logger.addHandler(mem_handler)
        self._stat_logger.setLevel(logging.INFO)

    def _setup_info_logger(self, log_directory: str, log_capacity=1000000):
        """Setup the information logger"""
        handler = logging.FileHandler(log_directory + '/info.log', mode='w')
        formatter = logging.Formatter('%(levelname)s %(asctime)s - '
                                      '%(funcName)s - %(message)s',
                                      '%d-%m-%Y %H:%M:%S')
        handler.setFormatter(formatter)
        mem_handler = logging.handlers.MemoryHandler(log_capacity,
                                                     logging.ERROR,
                                                     handler)

        self._info_logger = logging.getLogger('info_logger')
        self._info_logger.addHandler(mem_handler)
        self._info_logger.setLevel(logging.DEBUG)

    def _log_status(self, log_msg):
        self._stat_logger.info(log_msg)

    def _log_info(self, log_msg):
        self._info_logger.debug(log_msg)

    def _logger_disabled(self, log_msg):
        pass
