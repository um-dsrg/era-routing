#!/usr/bin/env bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
baseDir="${scriptDir}/.."

# Defining constant variable
k=5
homeString="\${HOME}"
baseJobscriptsDir="${scriptDir}/../Jobscripts/Moea"

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

    ackConsideration="ConsideringAcks"
    for numFlows in "${numFlowsArray[@]}"; do
        flowsDir="${networkLoadDir}/Flows${numFlows}"

        for moeaAlgorithm in moea1 moea2; do

            jobscriptsDir="${baseJobscriptsDir}/${networkLoad}/${moeaAlgorithm^}"
            mkdir -p "${jobscriptsDir}"

            for pathsAlgorithm in Ksp Red; do
                for flowsetNumber in {1..5}; do
                    jobName="${moeaAlgorithm}_${networkLoad}_${numFlows}_${pathsAlgorithm}_Ca_${flowsetNumber}"
                    jobFile="${jobscriptsDir}/${jobName}.pbs"
                    echo "${jobFile}"
                    rm -f "${jobFile}"  # Remove the file just in case it already exists

                    # Setup the logs directory
                    logsDir="${jobscriptsDir}/logs/${jobName}"
                    mkdir -p "${logsDir}"
                    logsDir=$(readlink -f "${logsDir}")
                    logsDir=${logsDir/#$HOME/${homeString}}

                    # Retrieve the path file
                    pathFile="${flowsDir}/Paths/${pathsAlgorithm}/K_${k}/${pathsAlgorithm,,}${flowsetNumber}.xml"
                    pathFile=$(readlink -f "${pathFile}")
                    pathFile=${pathFile/#$HOME/${homeString}}

                    resultsDir="${flowsDir}/Moea/${moeaAlgorithm^}/${ackConsideration}/${pathsAlgorithm}/K_${k}"
                    mkdir -p "${resultsDir}"

                    if [ ${moeaAlgorithm} = "moea1" ]; then
                        resultFile="${resultsDir}/moeaI_${flowsetNumber}.xml"
                    elif [ ${moeaAlgorithm} = "moea2" ]; then
                        resultFile="${resultsDir}/moeaII_${flowsetNumber}.xml"
                    fi

                    resultFile=$(readlink -f "${resultFile}")
                    resultFile=${resultFile/#$HOME/${homeString}}

                    # Jobscript contents
                    {
                        printf "#!/usr/bin/env bash\n";
                        printf "#\n";
                        printf "#PBS -q medium\n";
                        printf "#\n";
                        printf "#PBS -N %s\n" "${jobName}";
                        printf "#\n";
                        printf "#PBS -l ncpus=1\n";
                        printf "#\n";
                        printf "#PBS -m a\n";
                        printf "#\n";
                        printf "#PBS -M noel.farrugia.09@um.edu.mt\n";
                        printf "#\n";
                        printf "#PBS -e localhost:\${HOME}/logs/\${PBS_JOBNAME}.err.txt\n";
                        printf "#PBS -o localhost:\${HOME}/logs/\${PBS_JOBNAME}.out.txt\n";
                        printf "\n";
                        printf "python3 \"\${HOME}/Documents/Git/Development/GeneticAlgorithm/ga.py\" \\";
                        printf "\n";
                        printf "    --input=\"%s\" \\" "${pathFile}";
                        printf "\n";
                        printf "    --output=\"%s\" \\" "${resultFile}";
                        printf "\n";
                        printf "    --algorithm=\"nsga2\" \\";
                        printf "\n";
                        printf "    --num_generations 400 \\";
                        printf "\n";
                        printf "    --pop_size 800 \\";
                        printf "\n";
                        printf "    --populationGenerator=\"RandomFlowAllocation\" \\";
                        printf "\n";
                        printf "    --prob_crossover 0.9 \\";
                        printf "\n";
                        printf "    --prob_mutation 0.2 \\";
                        printf "\n";
                        printf "    --mutation_fraction 0.2 \\";
                        printf "\n";
                        printf "    --considerAcks \\";
                        printf "\n";
                        printf "    --status_log \\";
                        printf "\n";
                        printf "    --log_directory=\"%s\" \\" "${logsDir}";
                        printf "\n";
                        if [ ${moeaAlgorithm} = "moea1" ]; then
                            printf "    --mutationFunctions=\"MaximiseFlow, MinimiseCost, MinimisePathsUsed\" \\";
                            printf "\n";
                            printf "    --mutationFunctionProbability=\"0.335, 0.335, 0.33\" \\";
                            printf "\n";
                            printf "    --objectives \\";
                            printf "\n";
                            printf "    \"net_flow, 1, _calculate_total_network_flow, _get_network_flow_upper_bound\" \\";
                            printf "\n";
                            printf "    \"delay_met, 1, _calculate_delay_distribution_metric, _get_delay_distribution_upper_bound\" \\";
                            printf "\n";
                            printf "    \"tot_flow_splits, -1, _calculate_flow_splits_metric, _get_flow_splits_upper_bound\"";
                        elif [ ${moeaAlgorithm} = "moea2" ]; then
                            printf "    --mutationFunctions=\"MaximiseFlow, MinimiseMaxDelay\" \\";
                            printf "\n";
                            printf "    --mutationFunctionProbability=\"0.5, 0.5\" \\";
                            printf "\n";
                            printf "    --objectives \\";
                            printf "\n";
                            printf "    \"net_flow, 1, _calculate_total_network_flow, _get_network_flow_upper_bound\" \\";
                            printf "\n";
                            printf "    \"max_delay_met, -1, _calculate_max_delay_metric, _get_max_delay_upper_bound\"";
                        fi
                        printf "\n";
                    } >> "${jobFile}"
                done
            done
        done
    done
done

# # # # Generate the parallelCommands text and job submission file # # #
parallelCommandsFile=${baseJobscriptsDir}/parallelCommands.txt
rm -f "${parallelCommandsFile}"

find "${baseJobscriptsDir}" -name "*.pbs"|while read -r fileName; do
    absFilePath=$(readlink -f "${fileName}")
    absFilePath=${absFilePath/#$HOME/${homeString}}
    printf "bash %s\n" "${absFilePath}" >> "${parallelCommandsFile}"
done

parallelSubmitFile=${baseJobscriptsDir}/submitCommands.sh
rm -f "${parallelSubmitFile}"

{
    printf "#!/usr/bin/env bash";
    printf "\n";
    printf "scriptDir=\"\$( cd \"\$( dirname \"\${BASH_SOURCE[0]}\" )\" && pwd )\"";
    printf "\n\n";
    printf "ulimit -Sv 4000000  # 4GB\n";
    printf "parallel --verbose --jobs 40 < \"\${scriptDir}/parallelCommands.txt\"";
    printf "\n";
} >> "${parallelSubmitFile}"

chmod +x "${parallelSubmitFile}"
