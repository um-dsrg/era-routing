#!/usr/bin/env bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
baseDir="${scriptDir}/.."

flowGenScript="${HOME}/Documents/Git/Development/SimulationSetup/generate_random_flow_set.py"
baseGeantLgf="${HOME}/Documents/Git/Development/SimulationSetup/base_geant.lgf"

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
        flowsetsDir="${networkLoadDir}/Flows${numFlows}/Flowsets"
        mkdir -p "${flowsetsDir}"

        for flowsetNumber in {1..5}; do
            echo "Network Load: ${networkLoadStr} Number of Flows: ${numFlows} Flowset Number: ${flowsetNumber}"
            python3 "${flowGenScript}" \
                --input_lgf_path="${baseGeantLgf}" \
                --output_lgf_path="${flowsetsDir}/flowset${flowsetNumber}.lgf" \
                --num_flows="${numFlows}" \
                --scaling_factor=30 \
                --flow_protocol=T \
                --terminal_to_switch_dr=10000 \
                --network_load="${networkLoadStr}"
        done
    done
done
