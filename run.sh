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
for numberOfSmartMeters in "${numbersOfSmartMeters[@]}"; do
    for trial in $(seq 1 ${numTrials}); do
        if [ `expr ${run} % ${numCores}` -eq 1 ] && [ $run != 1 ]
        then
            echo "Waiting previous simulations to finish..."
            wait
        fi

        echo "Starting simulation with ${numberOfSmartMeters} smart meters of trial ${trial}"
        trialOutputDir=${outputsDir}/${numberOfSmartMeters}-meters/trial${trial}
        mkdir -p ${trialOutputDir}
        (./waf --cwd="${trialOutputDir}" --run "${program} \
            --numberOfSmartMeters=${numberOfSmartMeters} \
            --maxSimulationTimeInSeconds=${maxSimulationTimeInSeconds} \
            --maxElapsedClockTimeInSeconds=${maxElapsedClockTimeInSeconds}" \
            > ${trialOutputDir}/out) &
        
        sleep 1 # sleep between calls to avoid races in ns-3
        run=$((run+1))
    done
done
wait
echo "Done."

# go back to the project directory
cd - >/dev/null

# compute the results
./results.py
