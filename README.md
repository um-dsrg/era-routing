# Description

Repository containing the Evolutionary Algorithm and path generation algorithms
used and published in XX.
The `xml` generated files are used as an input to the
[ns-3](https://github.com/um-dsrg/era-ns3).
The results and flowsets are uploaded to Zenodo and can be found XX.

# Folder Structure

## GeneticAlgorithm

Contains all the source code for the Evolutionary Routing Algorithm.

## LpSolver

Contains all the source code to solve the Multi-Commodity Flow problem using the
LEMON library and GLPK.

## NetworkTopologyLgfToXml

Convert a Lemon Graph File (LGF) to an XML format required by `ns-3`.

## PathSelector

Implementation of the K-Shortest Path and the Relaxed K Shortest Path algorithm.

## PcLp

Contains all the source code to solve the Path Constrained Multi-Commodity Flow
problem using the LEMON library and GLPK.

## SimulationSetup

Contains python utilities to generate a random flow set from the base GÉANT
network topology graph using the Lemon Graph File (LGF) format.

## Scripts

Contains a number of `bash` scripts used to generate the flow sets and run
simulations.

## Jobscripts

Contains the jobscripts with the exact commands executed to generate the
results.
