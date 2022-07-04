#!/usr/bin/env bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

baseDir="${scriptDir}/.."
pathSelectorExe="${HOME}/Documents/Git/Development/PathSelector/build/release/pathSelector"

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

    for numFlows in "${numFlowsArray[@]}"; do
        flowsDir="${networkLoadDir}/Flows${numFlows}"
        flowsetsDir="${flowsDir}/Flowsets"
        algorithm="Red"
        algoUpperCase="RED"
        algoLowerCase="red"

        for k in 1 2 3 4 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25; do
            pathsDir="${flowsDir}/Paths/${algorithm}/K_${k}"
            mkdir -p "${pathsDir}"

            for flowsetNumber in {1..5}; do
                echo "Network Load: ${networkLoadStr} Number of Flows: ${numFlows} Algorithm: ${algorithm} K: ${k} Flowset Number: ${flowsetNumber}"
                ${pathSelectorExe} \
                    --input="${flowsetsDir}/flowset${flowsetNumber}.lgf" \
                    --output="${pathsDir}/${algoLowerCase}${flowsetNumber}.xml" \
                    --pathSelectionAlgorithm="${algoUpperCase}" \
                    --globalK="${k}"
            done
        done
    done
done
