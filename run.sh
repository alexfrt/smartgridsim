#!/bin/bash

set -e

# read the simulation variables
. ./vars.sh

# clean the outputs directory
rm -rf ${outputsDir}
mkdir -p ${outputsDir}

# go to the ns-3 directory
cd ../.. >/dev/null

# do a fake run to compile the simulation program
./waf --cwd="/tmp" --run "${program} --maxSimulationTimeInSeconds=0"

# run the simulation
run=1
for aggregationPercentage in "${aggregationPercentages[@]}"; do
    for numberOfSmartMeters in "${numbersOfSmartMeters[@]}"; do
        for trial in $(seq 1 ${numTrials}); do
            if [ `expr ${run} % ${numCores}` -eq 1 ] && [ $run != 1 ]
            then
                echo "Waiting previous simulations to finish..."
                wait
            fi

            echo "Starting simulation with aggregation percentage of ${aggregationPercentage}% and ${numberOfSmartMeters} smart meters on trial ${trial}"
            trialOutputDir=${outputsDir}/agg${aggregationPercentage}/${numberOfSmartMeters}-meters/trial${trial}
            mkdir -p ${trialOutputDir}
            (NS_GLOBAL_VALUE="RngRun=${run}" ./waf --cwd="${trialOutputDir}" --run "${program} \
                --numberOfSmartMeters=${numberOfSmartMeters} \
                --aggregationPercentage=${aggregationPercentage} \
                --maxSimulationTimeInSeconds=${maxSimulationTimeInSeconds} \
                --maxElapsedClockTimeInSeconds=${maxElapsedClockTimeInSeconds}" \
                > ${trialOutputDir}/out) &

            sleep 1 # sleep between calls to avoid races in ns-3
            run=$((run+1))
        done
    done
done
wait
echo "Done."

# go back to the project directory
cd - >/dev/null

# compute the results
MPLBACKEND="SVG" ./results.py
