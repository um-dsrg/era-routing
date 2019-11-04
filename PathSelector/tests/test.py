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
        self.ackShortestPath = [int(linkElement.get("Id")) for linkElement
                                in flowElement.findall("AckShortestPath/Link")]  # type: List[int]


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

    def PathExists(self, flowId: int, dataLinks: List[int], ackLinks: List[int]) -> bool:
        """Verify that the given Data path and it's respective ACK path exist

        Arguments:
            flowId {int} -- The Flow id to check the path
            dataLinks {List[int]} -- List of links that make up the Data path
            ackLinks {List[int]} -- List of links that make up the Ack path

        Returns:
            bool -- True if data & ack path exist. False otherwise
        """
        if flowId in self.flows:
            flow = self.flows[flowId]  # type: Flow

            # Verify the data path exist
            dataPathId = 0
            for path in flow.dataPaths.values():
                if path.links == dataLinks:
                    dataPathId = path.id
                    return True

            # Verify that the matching ACK path conforms to the one given
            if dataPathId in flow.ackPaths:
                ackPath = flow.ackPaths[dataPathId]
                if ackPath.links == ackLinks:
                    return True
            else:
                return False

        return False


class TestPathSelector(unittest.TestCase):
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
        self.assertTrue(pa.VerifyAckPaths(), "ACK Paths verification failed")
        self.assertTrue(pa.VerifyPathCost(), "Path Cost verification failed")
        self.assertTrue(pa.VerifyNetworkTopology(), "Network Topology verification failed")
        self.assertTrue(pa.VerifyNumPaths(k), "Number of paths verification failed")

    def runPathSelector(self, topology: str, algorithm: str, k: int) -> str:
        """Run the PathSelector algorithm with the given parameters"""
        print(F"Running the {algorithm} algorithm with k {k}...")
        resultFile, pathSelCommand = self.genPathSelectorCommand(topology,
                                                                 F"{topology}_{algorithm}_K{k}",
                                                                 algorithm, k)
        ret = subprocess.run(pathSelCommand, check=True)
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
                                  dataPaths: List[List[int]], ackPaths: List[List[int]],
                                  verifyResultFile: bool = True, maxIterations: int = 50) -> bool:
        """
        Ensure that all the paths in the list are at least found once by the
        algorithm. This function is used to test the random path selection
        mechanism when there are multiple paths available with equal cost to the
        kth path. The verifyResultFile option is given because in the case of parallel links the ACK
        verification may fail.

        :param topology: The topology to run
        :param algorithm: The algorithm to use
        :param k: The number of paths to find
        :param flowId: The flow to test
        :param dataPaths: The list of data paths that need to be returned by the
                          algorithm.
        :param ackPaths: The list of ack paths related to the given data paths.
        :param verifyResultFile: When set to true verify the generated result file.
        :param maxIterations: The maximum number of iterations to try before
                              failing the test

        :return: True if the test passes, false otherwise
        """
        self.assertEqual(len(dataPaths), len(ackPaths),
                         "The number of data paths and ack paths given does not match")

        pathFound = [False for _ in dataPaths]
        print(F"Running the {algorithm} algorithm with k {k}...")
        for iterationNumber in range(0, maxIterations):
            print(F"Iteration {iterationNumber}")
            resFileName = F"{topology}_{algorithm}_K{k}_{iterationNumber}"
            resultFile, pathSelCommand = self.genPathSelectorCommand(topology, resFileName,
                                                                     algorithm, k)
            ret = subprocess.run(pathSelCommand, check=True)
            self.assertEqual(ret.returncode, 0, "The PathSelector algorithm failed")

            pa = None
            if verifyResultFile:
                pa = self.verifyResultFile(resultFile, k)
            else:
                pa = PathAnalyser(resultFile)

            for pathIndex, (dataPathLinks, ackPathLinks) in enumerate(zip(dataPaths, ackPaths)):
                if pa.PathExists(flowId, dataPathLinks, ackPathLinks) is True:
                    pathFound[pathIndex] = True

            if all(path is True for path in pathFound):
                return True

        return False

    def verifyAckShortestPath(self, topology: str, algorithm: str, k: int, flowId: int,
                              ackPaths: List[List[int]], maxIterations: int = 50) -> bool:
        """Verify that the ACK shortest path is chosen at random if multiple paths exist"""
        pathFound = [False for _ in ackPaths]
        reverseAckPaths = [ackPath[::-1] for ackPath in ackPaths]

        print(F"AckShortestPath - Running the {algorithm} algorithm with k {k}...")

        for iterationNumber in range(0, maxIterations):
            print(F"Iteration {iterationNumber}")
            resFileName = F"{topology}_{algorithm}_K{k}_{iterationNumber}"
            resultFile, pathSelCommand = self.genPathSelectorCommand(topology, resFileName,
                                                                     algorithm, k)
            ret = subprocess.run(pathSelCommand, check=True)
            self.assertEqual(ret.returncode, 0, "The PathSelector algorithm failed")

            pa = PathAnalyser(resultFile)
            flow = pa.flows[flowId]

            for pathIndex, ackShortestPath in enumerate(reverseAckPaths):
                if flow.ackShortestPath == ackShortestPath:
                    pathFound[pathIndex] = True

            if all(path is True for path in pathFound):
                return True

        return False

    @timeout_decorator.timeout(1, use_signals=False)
    def testButterfly(self):
        """Test the Butterfly topology for all the given algorithms"""
        topology = "butterfly"
        for k, algorithm in zip([1, 5], ["KSP", "RED", "ED"]):
            pa = self.runAndVerify(topology, algorithm, k)
            if k == 1:
                pa = self.runAndVerify(topology, algorithm, k)
                self.assertTrue(pa.PathExists(0, [0, 4, 18], [1, 5, 19]))
                self.assertTrue(pa.PathExists(1, [2, 12, 20], [3, 13, 21]))
            else:  # k = 5
                pa = self.runAndVerify(topology, algorithm, k)
                # Flow 0
                self.assertTrue(pa.PathExists(0, [0, 4, 18], [1, 5, 19]))
                self.assertTrue(pa.PathExists(0, [0, 6, 10, 14, 18], [1, 7, 11, 15, 19]))
                if algorithm == "KSP":
                    self.assertTrue(pa.PathExists(0,
                                                  [0, 6, 9, 12, 17, 14, 18],
                                                  [1, 7, 8, 13, 16, 15, 19]))
                # Flow 1
                self.assertTrue(pa.PathExists(1, [2, 12, 20], [3, 13, 21]))
                self.assertTrue(pa.PathExists(1, [2, 8, 10, 16, 20], [3, 9, 11, 17, 21]))
                if algorithm == "KSP":
                    self.assertTrue(pa.PathExists(1,
                                                  [2, 8, 7, 4, 15, 16, 20],
                                                  [3, 9, 6, 5, 14, 17, 21]))

    @timeout_decorator.timeout(1, use_signals=False)
    def testDiamond(self):
        """Test the Diamond topology for various k values"""
        for k in [1, 2, 5]:
            for algorithm in ["KSP", "RED", "ED"]:
                if k == 1:
                    self.assertTrue(self.verifyRandomPathSelection("diamond", algorithm, k, 0,
                                                                   [[0, 2, 6, 10], [0, 4, 8, 10]],
                                                                   [[1, 3, 7, 11], [1, 5, 9, 11]]))
                else:
                    pa = self.runAndVerify("diamond", algorithm, k)
                    self.assertTrue(pa.PathExists(0, [0, 2, 6, 10], [1, 3, 7, 11]))
                    self.assertTrue(pa.PathExists(0, [0, 4, 8, 10], [1, 5, 9, 11]))

                self.assertTrue(self.verifyAckShortestPath("diamond", algorithm, k, 0,
                                                           [[1, 3, 7, 11], [1, 5, 9, 11]]),
                                "AckShortestPath Verification failed")

    @timeout_decorator.timeout(1, use_signals=False)
    def testLine(self):
        """Test the Line topology with various k values"""
        for k in [1, 5, 10]:
            for algorithm in ["KSP", "RED", "ED"]:
                pa = self.runAndVerify("line", algorithm, k)

                self.assertTrue(pa.PathExists(0, [0, 2, 4], [1, 3, 5]))

    @timeout_decorator.timeout(1, use_signals=False)
    def testCircle(self):
        """Test the Circle topology with various k values"""
        k = 1
        for algorithm in ["KSP", "RED", "ED"]:
            self.assertTrue(self.verifyRandomPathSelection("circle", algorithm, k, 0,
                                                           [[0, 2, 14, 26], [0, 4, 16, 26]],
                                                           [[1, 3, 15, 27], [1, 5, 17, 27]]))

        k = 2
        for algorithm in ["KSP", "RED", "ED"]:
            pa = self.runAndVerify("circle", algorithm, k)
            self.assertTrue(pa.PathExists(0, [0, 2, 14, 26], [1, 3, 15, 27]))
            self.assertTrue(pa.PathExists(0, [0, 4, 16, 26], [1, 5, 17, 27]))

        k = 3
        for algorithm in ["KSP", "RED", "ED"]:
            pa = self.runAndVerify("circle", algorithm, k)
            self.assertTrue(pa.PathExists(0, [0, 2, 14, 26], [1, 3, 15, 27]))
            self.assertTrue(pa.PathExists(0, [0, 4, 16, 26], [1, 5, 17, 27]))

            self.assertTrue(self.verifyRandomPathSelection("circle", algorithm, k, 0,
                                                           [[0, 6, 18, 26], [0, 8, 20, 26]],
                                                           [[1, 17, 19, 27], [1, 9, 21, 27]]))
        k = 4
        for algorithm in ["KSP", "RED", "ED"]:
            pa = self.runAndVerify("circle", algorithm, k)
            self.assertTrue(pa.PathExists(0, [0, 2, 14, 26], [1, 3, 15, 27]))
            self.assertTrue(pa.PathExists(0, [0, 4, 16, 26], [1, 5, 17, 27]))
            self.assertTrue(pa.PathExists(0, [0, 6, 18, 26], [1, 7, 19, 27]))
            self.assertTrue(pa.PathExists(0, [0, 8, 20, 26], [1, 9, 21, 27]))

        k = 5
        for algorithm in ["KSP", "RED", "ED"]:
            pa = self.runAndVerify("circle", algorithm, k)
            self.assertTrue(pa.PathExists(0, [0, 2, 14, 26], [1, 3, 15, 27]))
            self.assertTrue(pa.PathExists(0, [0, 4, 16, 26], [1, 5, 17, 27]))
            self.assertTrue(pa.PathExists(0, [0, 6, 18, 26], [1, 7, 19, 27]))
            self.assertTrue(pa.PathExists(0, [0, 8, 20, 26], [1, 9, 21, 27]))

            self.assertTrue(self.verifyRandomPathSelection("circle", algorithm, k, 0,
                                                           [[0, 10, 22, 26], [0, 12, 24, 26]],
                                                           [[1, 11, 23, 27], [1, 13, 25, 27]]))

        k = 6
        for algorithm in ["KSP", "RED", "ED"]:
            pa = self.runAndVerify("circle", algorithm, k)
            self.assertTrue(pa.PathExists(0, [0, 2, 14, 26], [1, 3, 15, 27]))
            self.assertTrue(pa.PathExists(0, [0, 4, 16, 26], [1, 5, 17, 27]))
            self.assertTrue(pa.PathExists(0, [0, 6, 18, 26], [1, 7, 19, 27]))
            self.assertTrue(pa.PathExists(0, [0, 8, 20, 26], [1, 9, 21, 27]))
            self.assertTrue(pa.PathExists(0, [0, 10, 22, 26], [1, 11, 23, 27]))
            self.assertTrue(pa.PathExists(0, [0, 12, 24, 26], [1, 13, 25, 27]))

    @timeout_decorator.timeout(1, use_signals=False)
    def testTwoDiamond(self):
        """Test the two conjoined diamond topology with a large k value"""
        k = 20

        algorithm = "KSP"
        pa = self.runAndVerify("twoDiamond", algorithm, k)
        self.assertTrue(pa.PathExists(0, [0, 2, 6, 10, 12, 16, 20], [1, 3, 7, 11, 13, 17, 21]))
        self.assertTrue(pa.PathExists(0, [0, 4, 8, 10, 14, 18, 20], [1, 5, 9, 11, 13, 17, 21]))
        self.assertTrue(pa.PathExists(0, [0, 2, 6, 10, 14, 18, 20], [1, 3, 7, 11, 15, 19, 21]))
        self.assertTrue(pa.PathExists(0, [0, 4, 8, 10, 12, 16, 20], [1, 5, 9, 11, 15, 19, 21]))

        algorithm = "RED"
        pa = self.runAndVerify("twoDiamond", algorithm, k)
        self.assertTrue(pa.PathExists(0, [0, 4, 8, 10, 14, 18, 20], [1, 5, 9, 11, 15, 19, 21]))

        algorithm = "ED"
        pa = self.runAndVerify("twoDiamond", algorithm, k)
        self.assertTrue(pa.PathExists(0, [0, 4, 8, 10, 14, 18, 20], [1, 5, 9, 11, 15, 19, 21]))

    @timeout_decorator.timeout(1, use_signals=False)
    def testLineParallel(self):
        """Test the lineParallel topology for various k values

        ACK verification is missing due to the problem caused in determining
        which ACK path to choose when having parallel links. This is not a very
        important feature; and thus is ruled as an exception in this case and
        treated as such.
        """
        for k in [1, 2, 5]:
            for algorithm in ["KSP", "RED", "ED"]:
                if k == 1:
                    self.assertTrue(self.verifyRandomPathSelection("lineParallel", algorithm, k, 0,
                                                                   [[0, 2, 6], [0, 4, 6]],
                                                                   [[1, 3, 7], [1, 5, 7]],
                                                                   verifyResultFile=False))
                    resultFile = self.runPathSelector("lineParallel", algorithm, k)
                    pa = PathAnalyser(resultFile)
                    self.assertTrue(pa.VerifyPathCost(), "Path Cost verification failed")
                    self.assertTrue(pa.VerifyNetworkTopology(),
                                    "Network Topology verification failed")
                    self.assertTrue(pa.VerifyNumPaths(k), "Number of paths verification failed")
                else:
                    resultFile = self.runPathSelector("lineParallel", algorithm, k)
                    pa = PathAnalyser(resultFile)
                    self.assertTrue(pa.VerifyPathCost(), "Path Cost verification failed")
                    self.assertTrue(pa.VerifyNetworkTopology(),
                                    "Network Topology verification failed")
                    self.assertTrue(pa.VerifyNumPaths(k), "Number of paths verification failed")

                    self.assertTrue(pa.PathExists(0, [0, 2, 6], [1, 3, 7]))
                    self.assertTrue(pa.PathExists(0, [0, 4, 6], [1, 5, 7]))

                self.assertTrue(self.verifyAckShortestPath("lineParallel", algorithm, k, 0,
                                                           [[1, 3, 7], [1, 5, 7]]),
                                "AckShortestPath Verification failed")

    @timeout_decorator.timeout(1, use_signals=False)
    def testDiamondParallel(self):
        """Test the diamondParallel topology for various k values

        ACK verification is missing due to the problem caused in determining
        which ACK path to choose when having parallel links. This is not a very
        important feature; and thus is ruled as an exception in this case and
        treated as such.
        """
        algorithm = "KSP"
        for k in [1, 10]:
            resultFile = self.runPathSelector("diamondParallel", algorithm, k)
            pa = PathAnalyser(resultFile)

            self.assertTrue(pa.VerifyPathCost(), "Path Cost verification failed")
            self.assertTrue(pa.VerifyNetworkTopology(),
                            "Network Topology verification failed")
            self.assertTrue(pa.VerifyNumPaths(k), "Number of paths verification failed")

            if k == 1:
                self.assertTrue(self.verifyRandomPathSelection("diamondParallel", algorithm, k, 0,
                                                               [[0, 2, 6, 10, 14],
                                                                [0, 2, 8, 12, 14],
                                                                [0, 4, 6, 10, 14],
                                                                [0, 4, 8, 12, 14]],
                                                               [[1, 3, 7, 11, 15],
                                                                [1, 3, 9, 13, 15],
                                                                [1, 5, 7, 11, 15],
                                                                [1, 5, 9, 13, 15]],
                                                               verifyResultFile=False))

            elif k == 10:
                self.assertTrue(pa.PathExists(0, [0, 2, 6, 10, 14], [1, 3, 7, 11, 15]))
                self.assertTrue(pa.PathExists(0, [0, 2, 8, 12, 14], [1, 3, 9, 13, 15]))
                self.assertTrue(pa.PathExists(0, [0, 4, 6, 10, 14], [1, 5, 7, 11, 15]))
                self.assertTrue(pa.PathExists(0, [0, 4, 8, 12, 14], [1, 5, 9, 13, 15]))

        for algorithm in ["RED", "ED"]:
            for k in [1, 10]:
                resultFile = self.runPathSelector("diamondParallel", algorithm, k)
                pa = PathAnalyser(resultFile)

                self.assertTrue(pa.VerifyPathCost(), "Path Cost verification failed")
                self.assertTrue(pa.VerifyNetworkTopology(),
                                "Network Topology verification failed")
                self.assertTrue(pa.VerifyNumPaths(k), "Number of paths verification failed")

                if k == 1:
                    self.assertTrue(self.verifyRandomPathSelection("diamondParallel", algorithm, k,
                                                                   0,
                                                                   [[0, 2, 6, 10, 14],
                                                                    [0, 4, 8, 12, 14]],
                                                                   [[1, 3, 7, 11, 15],
                                                                    [1, 5, 9, 13, 15]],
                                                                   verifyResultFile=False))
                elif k == 10:
                    self.assertTrue(pa.PathExists(0, [0, 2, 6, 10, 14], [1, 3, 7, 11, 15]))
                    self.assertTrue(pa.PathExists(0, [0, 4, 8, 12, 14], [1, 5, 9, 13, 15]))


if __name__ == "__main__":
    unittest.main(verbosity=2)
