# Description

Repository containing the Evolutionary Algorithm and path generation algorithms
used and published in XX.
The `xml` generated files are used as an input to the
[ns-3](https://github.com/um-dsrg/era-ns3).

# Folder Structure

## GeneticAlgorithm

Contains all the source code for the Evolutionary Routing Algorithm.

## LpSolver

Contains all the source code to solve the Multi-Commodity Flow problem using the
LEMON library and GLPK.

## PcLp

Contains all the source code to solve the Path Constrained Multi-Commodity Flow
problem using the LEMON library and GLPK.

## PathSelector

Implementation of the K-Shortest Path and the Relaxed K Shortest Path algorithm.

## SimulationSetup

Contains python utilities to generate a random flow set from the base GÉANT
network topology graph using the Lemon Graph File (LGF) format.

## NetworkTopologyLgfToXml

Convert a Lemon Graph File (LGF) to an XML format required by `ns-3`.
