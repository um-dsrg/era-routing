#!/usr/bin/env bash
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

ulimit -Sv 8000000  # 8GB
parallel --verbose --jobs 20 < "${scriptDir}/parallelCommands.txt"
