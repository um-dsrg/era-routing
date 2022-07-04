#!/usr/bin/env bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

baseDir="${scriptDir}/.."
lpExe="${HOME}/Documents/Git/Development/LpSolver/build/release/lp-solver"

for networkLoad in Low Med High; do
    networkLoadDir="${baseDir}/NetworkLoad/${networkLoad}"

    if [ "${networkLoad}" == "Low" ]; then
        networkLoadStr="Low"
        numFlowsArray=(50 100 150 200 250 300)
    elif [ "${networkLoad}" == "Med" ]; then
        networkLoadStr="Medium"
        numFlowsArray=(50 100 150 200 250 300)
    elif [ "${networkLoad}" == "High" ]; then
        networkLoadStr="High"
        numFlowsArray=(50 100 150)
    fi

    ackConsideration="NotConsideringAcks"
    for numFlows in "${numFlowsArray[@]}"; do

        flowsDir="${networkLoadDir}/Flows${numFlows}"
        flowsetsDir="${flowsDir}/Flowsets"

        resultsDir="${flowsDir}/UnboundLp/${ackConsideration}"
        mkdir -p "${resultsDir}"

        for flowsetNumber in {1..5}; do
            echo "Network Load ${networkLoad} | Number of Flows: ${numFlows} | Flowset Number: ${flowsetNumber}"

            graphFile="${flowsetsDir}/flowset${flowsetNumber}.lgf"

            "${lpExe}" \
                --lgfPath="${graphFile}" \
                --xmlLogPath="${resultsDir}/rsUnbound${flowsetNumber}.xml" \
                --solverConfig="mf"
        done
    done
done
