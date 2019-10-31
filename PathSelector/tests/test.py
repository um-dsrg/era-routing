#!/usr/bin/env python3
"""Test the functionality of the PathSelector algorithm"""

import math
import os
import pathlib
import subprocess
import unittest
from typing import Tuple, List, Dict

import timeout_decorator
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
        self.resultFileRoot = etree.parse(resFilePath, xmlParser).getroot()

        self.flows = {int(flowElement.get("Id")): Flow(flowElement) for flowElement in
                      self.resultFileRoot.findall("FlowDetails/Flow")}
        self.linkProperties = {int(linkElement.get("Id")): float(linkElement.get("Cost"))
                               for linkElement in self.resultFileRoot.findall("LinkDetails/Link")}

    def VerifyNetworkTopology(self) -> bool:
        """Verify the created Network Topology element"""
        for linkElement in self.resultFileRoot.findall("NetworkTopology/Link"):
            linkCost = float(linkElement.get("Delay"))

            srcNodes = list()
            dstNodes = list()

            for linkElementElement in linkElement.findall("LinkElement"):
                linkId = int(linkElementElement.get("Id"))
                if self.linkProperties[linkId] != linkCost:
                    return False

                srcNodes.append(int(linkElementElement.get("SourceNode")))
                dstNodes.append(int(linkElementElement.get("DestinationNode")))

            if srcNodes[0] != dstNodes[1] or srcNodes[1] != dstNodes[0]:
                return False

        return True

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

    def VerifyAckPaths(self) -> bool:
        """Check that the ACK paths use the reverse links of the data paths"""
        oppositeLink = dict()

        for linkElement in self.resultFileRoot.findall("NetworkTopology/Link"):
            linkIds = [int(linkElementElement.get("Id")) for linkElementElement
                       in linkElement.findall("LinkElement")]
            oppositeLink[linkIds[0]] = linkIds[1]
            oppositeLink[linkIds[1]] = linkIds[0]

        for flow in self.flows.values():
            for pathId, dataPath in flow.dataPaths.items():
                flowAckPath = flow.ackPaths[pathId].links
                generatedAckPath = [oppositeLink[linkId] for linkId in dataPath.links]

                if flowAckPath != generatedAckPath:
                    return False

        return True

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
                               k: int) -> Tuple[str, list]:
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

        command = [F"{self.pathSelectorExePath}", "-i", F"{inputFile}", "-o", F"{outputFile}",
                   "--pathSelectionAlgorithm", F"{pathSelAlgorithm}", "--globalK", F"{k}"]

        return outputFile, command

    def verifySetup(self, pa: PathAnalyser, k: int):
        """Run all verification tests available"""
        self.assertTrue(pa.VerifyAckPaths())
        self.assertTrue(pa.VerifyPathCost())
        self.assertTrue(pa.VerifyNetworkTopology())
        self.assertTrue(pa.VerifyNumPaths(k))

    def runPathSelector(self, topology: str, algorithm: str, k: int) -> str:
        """Run the PathSelector algorithm with the given parameters"""
        print(F"Running the {algorithm} algorithm with k {k}...")
        resultFile, pathSelCommand = self.genPathSelectorCommand(topology,
                                                                 F"{topology}_{algorithm}_K{k}",
                                                                 algorithm, k)
        ret = subprocess.run(pathSelCommand)
        self.assertEqual(ret.returncode, 0, "The PathSelector algorithm failed")

        return resultFile

    def verifyResultFile(self, resultFile: str, k: int) -> PathAnalyser:
        """
        Run the path selector with the given details

        :param resultFile: The location to the generated result file
        :param k: The k value
        :return: Instance of the PathAnalyser class
        """
        pa = PathAnalyser(resultFile)
        self.verifySetup(pa, k)

        return pa

    def runAndVerify(self, topology: str, algorithm: str, k: int) -> PathAnalyser:
        """Run the PathSelector algorithm and verify the result file"""
        resultFile = self.runPathSelector(topology, algorithm, k)
        return self.verifyResultFile(resultFile, k)

    def verifyRandomPathSelection(self, topology: str, algorithm: str, k: int, flowId: int,
                                  paths: List[List[int]], maxIterations: int = 50) -> bool:
        """
        Ensure that all the paths in the list are at least found once by the
        algorithm. This function is used to test the random path selection
        mechanism when there are multiple paths available with equal cost to the
        kth path.

        :param topology: The topology to run
        :param algorithm: The algorithm to use
        :param k: The number of paths to find
        :param flowId: The flow to test
        :param paths: The list of paths to compare to
        :param maxIterations: The maximum number of iterations to try before
                              failing the test

        :return: True if the test passes, false otherwise
        """
        pathFound = [False for _ in paths]

        for _ in range(0, maxIterations):
            resultFile = self.runPathSelector(topology, algorithm, k)
            pa = self.verifyResultFile(resultFile, k)

            for pathIndex, pathLinks in enumerate(paths):
                if pa.DataPathExists(flowId, pathLinks) is True:
                    pathFound[pathIndex] = True

            if all(path is True for path in pathFound):
                return True

        return False

    @timeout_decorator.timeout(5, use_signals=False)
    def testDiamond(self):
        """Test the Diamond topology for various k values"""
        for k in [1, 2, 5]:
            for algorithm in ["KSP", "RED", "ED"]:
                if k == 1:
                    self.assertTrue(self.verifyRandomPathSelection("diamond", algorithm, k, 0,
                                                                   [[0, 2, 6, 10], [0, 4, 8, 10]]))
                else:
                    pa = self.runAndVerify("diamond", algorithm, k)
                    self.assertTrue(pa.DataPathExists(0, [0, 2, 6, 10]))
                    self.assertTrue(pa.DataPathExists(0, [0, 4, 8, 10]))
                    self.assertTrue(pa.AckPathExists(0, [1, 3, 7, 11]))
                    self.assertTrue(pa.AckPathExists(0, [1, 5, 9, 11]))

    @timeout_decorator.timeout(1, use_signals=False)
    def testLine(self):
        """Test the Line topology with various k values"""
        for k in [1, 5, 10]:
            for algorithm in ["KSP", "RED", "ED"]:
                pa = self.runAndVerify("line", algorithm, k)

                self.assertTrue(pa.DataPathExists(0, [0, 2, 4]))
                self.assertTrue(pa.AckPathExists(0, [1, 3, 5]))

    @timeout_decorator.timeout(1, use_signals=False)
    def testCircle(self):
        """Test the Circle topology with various k values"""
        # TODO: k = 1

        k = 2
        for algorithm in ["KSP", "RED", "ED"]:
            pa = self.runAndVerify("circle", algorithm, k)
            self.assertTrue(pa.DataPathExists(0, [0, 2, 14, 26]))
            self.assertTrue(pa.DataPathExists(0, [0, 4, 16, 26]))
            self.assertTrue(pa.AckPathExists(0, [1, 3, 15, 27]))
            self.assertTrue(pa.AckPathExists(0, [1, 5, 17, 27]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
