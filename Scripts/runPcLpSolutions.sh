#!/usr/bin/env bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

baseDir="${scriptDir}/.."
pclpExe="${HOME}/Documents/Git/Development/PcLp/build/release/pclp"

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

    for ackConsideration in ConsideringAcks NotConsideringAcks; do
        for numFlows in "${numFlowsArray[@]}"; do
            flowsDir="${networkLoadDir}/Flows${numFlows}"

            for algorithm in Ksp Red Ospf; do
                k=5
                if [ "${algorithm}" == "Ospf" ]; then
                    k=1
                fi

                pathsDir="${flowsDir}/Paths/${algorithm}/K_${k}"
                pclpDir="${flowsDir}/PcLp/${ackConsideration}/${algorithm}/K_${k}"
                mkdir -p "${pclpDir}"

                for flowsetNumber in {1..5}; do
                    echo "Network Load: ${networkLoadStr} Number of Flows: ${numFlows} Algorithm: ${algorithm} K: ${k} ${ackConsideration} Flowset Number: ${flowsetNumber}"

                    if [ "${ackConsideration}" == "ConsideringAcks" ]; then
                        ${pclpExe} \
                            --input="${pathsDir}/${algorithm,,}${flowsetNumber}.xml" \
                            --output="${pclpDir}/rs${algorithm}${flowsetNumber}.xml" \
                            --optimisationProblem="MinCost" \
                            --considerAckFlows >> "${scriptDir}/pclp.log"
                    elif [ "${ackConsideration}" == "NotConsideringAcks" ]; then
                        ${pclpExe} \
                            --input="${pathsDir}/${algorithm,,}${flowsetNumber}.xml" \
                            --output="${pclpDir}/rs${algorithm}${flowsetNumber}.xml" \
                            --optimisationProblem="MinCost" >> "${scriptDir}/pclp.log"
                    else
                        echo "The ACK consideration method ${ackConsideration} does not exist"
                        exit
                    fi
                done
            done
        done
    done
done
