#!/usr/bin/env python3
"""Test the functionality of the PathSelector algorithm"""

import os
import math
import pathlib
import unittest
from typing import Tuple, List, Dict

from lxml import etree


class Path:
    """Class representing a Path"""

    def __init__(self, pathElement):
        self.id = int(pathElement.get("Id"))
        self.cost = float(pathElement.get("Cost"))
        self.links = [int(linkElement.get("Id")) for linkElement in pathElement.findall("Link")]


class Flow:
    """Class representing a Flow"""

    def __init__(self, flowElement):
        self.id = int(flowElement.get("Id"))
        self.dataPaths = {int(pathElement.get("Id")): Path(pathElement) for pathElement in
                          flowElement.findall("Paths/Path")}  # type: Dict[int, Path]
        self.ackPaths = {int(pathElement.get("Id")): Path(pathElement) for pathElement in
                         flowElement.findall("AckPaths/Path")}  # type: Dict[int, Path]


class PathAnalyser:
    """
    Class that contains all the necessary functionality to analyse the path
    result file
    """

    def __init__(self, resFilePath: str):
        xmlParser = etree.XMLParser(remove_blank_text=True)
        resultFileRoot = etree.parse(resFilePath, xmlParser).getroot()

        self.flows = {int(flowElement.get("Id")): Flow(flowElement) for flowElement in
                      resultFileRoot.findall("FlowDetails/Flow")}
        self.linkProperties = {int(linkElement.get("Id")): float(linkElement.get("Cost")) for
                               linkElement in resultFileRoot.findall("LinkDetails/Link")}

    def DataPathExists(self, flowId: int, links: List[int]) -> bool:
        """Check if a data path with the same links exists for the given flow"""
        if flowId in self.flows:
            flow = self.flows[flowId]  # type: Flow

            for path in flow.dataPaths.values():
                if path.links == links:
                    return True

        return False

    def AckPathExists(self, flowId: int, links: List[int]) -> bool:
        """Check if an Ack path with the same links exists for the given flow"""
        if flowId in self.flows:
            flow = self.flows[flowId]  # type: Flow

            for path in flow.ackPaths.values():
                if path.links == links:
                    return True
        # else
        return False

    def VerifyNumPaths(self, k: int) -> bool:
        """Verify that no flow has more than k paths"""
        for flow in self.flows.values():
            if len(flow.dataPaths) > k or len(flow.ackPaths) > k:
                return False

        return True

    def VerifyPathCost(self) -> bool:
        """
        Ensure that the path cost is correct by verifying against the
        LinkDetails element
        """
        for flow in self.flows.values():
            for dataPath in flow.dataPaths.values():
                calculatedCost = sum([self.linkProperties[linkId] for linkId in dataPath.links])
                if not math.isclose(dataPath.cost, calculatedCost):
                    return False

        return True


class PathSelectorTestClass(unittest.TestCase):
    """Class to test the PathSelector Algorithm"""

    def setUp(self):
        """Function called to do any required setup before running a test

        Note that any exception raised by this method will be considered an
        error rather than a test failure.
        """
        homeDir = str(pathlib.Path.home())
        self.pathSelectorExePath = (F"{homeDir}/Documents/Git/Development/PathSelector/build/"
                                    "release/pathSelector")
        self.assertTrue(os.path.isfile(self.pathSelectorExePath),
                        F"The path selector executable was not found in {self.pathSelectorExePath}")
        self.baseDir = os.path.dirname(os.path.abspath(__file__))

    def genPathSelectorCommand(self, graph: str, resultFileName: str, pathSelAlgorithm: str,
                               k: int) -> Tuple[str, str]:
        """
        Generate the command to run the PathSelector

        :param graph: The name of the graph to use
        :param resultFileName: The name of the result file to generate
        :param pathSelAlgorithm: The path selection algorithm to use
        :param k: Number of paths to generate

        :return: (the full path to the output file, the path selector command)
        """
        inputFile = F"{self.baseDir}/graphs/{graph}.lgf"
        outputFile = F"{self.baseDir}/paths/{resultFileName}.xml"

        command = (F"{self.pathSelectorExePath} -i {inputFile} -o {outputFile} "
                   F"--pathSelectionAlgorithm {pathSelAlgorithm} --globalK {k}")

        return outputFile, command

    def testDiamondK_5(self):
        """Test the Diamond topology with k = 5"""
        k = 5
        outputFile, pathSelCommand = self.genPathSelectorCommand("diamond",
                                                                 F"diamond_KSP_K{k}", "KSP", k)
        os.system(pathSelCommand)

        pa = PathAnalyser(outputFile)

        self.assertTrue(pa.VerifyPathCost())
        self.assertTrue(pa.VerifyNumPaths(k))


if __name__ == "__main__":
    unittest.main(verbosity=2)
