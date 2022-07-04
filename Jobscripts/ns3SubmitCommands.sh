#!/usr/bin/env bash
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

ulimit -Sv 10500000  # 10.5GB
parallel --verbose --jobs 15 < "${scriptDir}/ns3ParallelCommands.txt"
