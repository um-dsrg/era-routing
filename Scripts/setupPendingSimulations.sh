#!/usr/bin/env bash

scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

python3 "${HOME}/Documents/Git/Development/Utilities/generatePendingJobList.py" \
    -i="${HOME}/Documents/Results/Dissertation/Results/Jobscripts/Moea" \
    -o="${HOME}/Documents/Results/Dissertation/Results/Jobscripts/Moea/parallelCommands.txt"
