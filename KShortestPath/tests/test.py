import os
import unittest
from lxml import etree
from typing import Dict
from pathlib import Path


class FlowPath:
    def __init__(self, pathElement, hasPathId=True):
        if hasPathId is True:
            self.id = int(pathElement.get('Id'))
        else:
            self.id = -1

        self.links = [int(linkElement.get('Id')) for linkElement in pathElement.findall('Link')]


class Flow:
    def __init__(self):
        self.id = 0               # type: int
        self.dataPaths = dict()   # type: Dict[int, FlowPath]
        self.ackPaths = dict()    # type: Dict[int, FlowPath]
        self.ackShortestPath = 0  # type: FlowPath


class ResultAnalyser:
    def __init__(self, resFilePath):
        self.flows = dict()  # type: Dict[int, Flow]
        self.parseFlows(resFilePath)

    def parseFlows(self, resFilePath):
        xml_parser = etree.XMLParser(remove_blank_text=True)
        resFileRoot = etree.parse(resFilePath, xml_parser).getroot()

        for flowElement in resFileRoot.findall('FlowDetails/Flow'):
            flow = Flow()
            flow.id = int(flowElement.get('Id'))

            # Parse the data paths
            for pathElement in flowElement.findall('Paths/Path'):
                path = FlowPath(pathElement)
                flow.dataPaths[path.id] = path

            # Parse the ack paths
            for ackPathElement in flowElement.findall('AckPaths/Path'):
                path = FlowPath(ackPathElement)
                flow.ackPaths[path.id] = path

            # Parse the ack paths
            ackShortestPathElement = flowElement.find('AckShortestPath')
            flow.ackShortestPath = FlowPath(ackShortestPathElement, hasPathId=False)

            self.flows[flow.id] = flow

    def getNumDataPaths(self, flowId):
        return len(self.flows[flowId].dataPaths)

    def getNumAckPaths(self, flowId):
        return len(self.flows[flowId].ackPaths)

    def dataPathExists(self, flowId, links):
        pathExists = False

        for flowPath in self.flows[flowId].dataPaths.values():
            pathLinks = flowPath.links
            if sorted(pathLinks) == sorted(links):
                pathExists = True
                break

        return pathExists

    def ackPathExists(self, flowId, links):
        pathExists = False

        for flowPath in self.flows[flowId].ackPaths.values():
            pathLinks = flowPath.links
            if sorted(pathLinks) == sorted(links):
                pathExists = True
                break

        return pathExists

    def compareAckShortestPath(self, flowId, links):
        return sorted(self.flows[flowId].ackShortestPath.links) == sorted(links)

    def comparePathIds(self, flowId, dataPath, ackPath):
        dataPathId = -1
        ackPathId = -1

        for flowPath in self.flows[flowId].dataPaths.values():
            if sorted(flowPath.links) == sorted(dataPath):
                dataPathId = flowPath.id
                break

        for flowPath in self.flows[flowId].ackPaths.values():
            if sorted(flowPath.links) == sorted(ackPath):
                ackPathId = flowPath.id
                break

        return dataPathId == ackPathId


class KspTestClass(unittest.TestCase):
    def setUp(self):
        home_dir = str(Path.home())
        self.kspExePath = home_dir + '/Repositories/Development/KShortestPath/build/release/ksp'
        self.assertTrue(os.path.isfile(self.kspExePath), 'The KSP release executable was not found')
        self.baseDir = os.path.dirname(os.path.abspath(__file__))

    def genKspCommand(self, inputFile, outputFile, globalK=0, perFlowK=False,
                      includeAllKEqualCostPaths=False):
        kspCommand = self.kspExePath + ' -i ' + inputFile + ' -o ' + outputFile

        if globalK > 0:
            kspCommand += ' --globalK ' + str(globalK)

        if perFlowK is True:
            kspCommand += ' --perFlowK'

        if includeAllKEqualCostPaths is True:
            kspCommand += ' --includeAllKEqualCostPaths'

        return kspCommand

    def testCircleThreePaths(self):
        inputGraphFile = self.baseDir + '/graphs/circleThreePaths.lgf'
        outputFile = self.baseDir + '/graphs/circleThreePaths.xml'
        kspCommand = self.genKspCommand(inputGraphFile, outputFile, perFlowK=True)
        os.system(kspCommand)

        ra = ResultAnalyser(outputFile)

        self.assertEqual(ra.getNumDataPaths(0), 3, 'The number of data paths is not equal to 3')
        self.assertEqual(ra.getNumAckPaths(0), 3, 'The number of ack paths is not equal to 3')

        self.assertTrue(ra.dataPathExists(0, [0, 3]), 'The data path 0 - 1 - 4 does not exist')
        self.assertTrue(ra.ackPathExists(0, [9, 6]), 'The ack path 4 - 1 - 0 does not exist')
        self.assertTrue(ra.comparePathIds(0, [9, 6], [0, 3]), 'The ack and data paths do not match')

        self.assertTrue(ra.dataPathExists(0, [1, 4]), 'The data path 0 - 2 - 4 does not exist')
        self.assertTrue(ra.ackPathExists(0, [10, 7]), 'The ack path 4 - 2 - 0 does not exist')
        self.assertTrue(ra.comparePathIds(0, [1, 4], [10, 7]), 'The ack and data paths do not match')

        self.assertTrue(ra.dataPathExists(0, [2, 5]), 'The data path 0 - 3 - 4 does not exist')
        self.assertTrue(ra.ackPathExists(0, [11, 8]), 'The ack path 4 - 3 - 0 does not exist')
        self.assertTrue(ra.comparePathIds(0, [2, 5], [11, 8]), 'The ack and data paths do not match')

        self.assertTrue(ra.compareAckShortestPath(0, [6, 9]), 'The shortest path ack does not match')

    def testButterflyPerFlowK(self):
        inputGraphFile = self.baseDir + '/graphs/butterfly.lgf'
        outputFile = self.baseDir + '/graphs/butterflyPerFlowK.xml'
        kspCommand = self.genKspCommand(inputGraphFile, outputFile, perFlowK=True, includeAllKEqualCostPaths=True)
        os.system(kspCommand)

        ra = ResultAnalyser(outputFile)

        # Flow 0
        self.assertEqual(ra.getNumDataPaths(0), 1, 'The number of data paths is not equal to 1')
        self.assertEqual(ra.getNumAckPaths(0), 1, 'The number of ack paths is not equal to 1')
        self.assertTrue(ra.dataPathExists(0, [0, 4, 18]), 'The data path 0 - 2 - 6 - 8 does not exist')
        self.assertTrue(ra.ackPathExists(0, [19, 5, 1]), 'The ack path 8 - 6 - 2 - 0 does not exist')
        self.assertTrue(ra.comparePathIds(0, [0, 4, 18], [19, 5, 1]), 'The ack and data paths do not match')

        # Flow 1
        self.assertEqual(ra.getNumDataPaths(1), 2, 'The number of data paths is not equal to 2')
        self.assertEqual(ra.getNumAckPaths(1), 2, 'The number of ack paths is not equal to 2')

        self.assertTrue(ra.dataPathExists(1, [2, 12, 20]), 'The data path 1 - 3 - 7 - 9 does not exist')
        self.assertTrue(ra.ackPathExists(1, [21, 13, 3]), 'The ack path 9 - 7 - 3 - 1 does not exist')
        self.assertTrue(ra.comparePathIds(1, [2, 12, 20], [21, 13, 3]), 'The ack and data paths do not match')

        self.assertTrue(ra.dataPathExists(1, [2, 8, 10, 16, 20]), 'The data path 2 - 8 - 10 - 16 - 20 does not exist')
        self.assertTrue(ra.ackPathExists(1, [21, 17, 11, 9, 3]), 'The ack path 20 - 16 - 10 - 8 - 2 does not exist')
        self.assertTrue(ra.comparePathIds(1, [2, 8, 10, 16, 20], [21, 17, 11, 9, 3]),
                        'The ack and data paths do not match')


if __name__ == '__main__':
    unittest.main(verbosity=2)
