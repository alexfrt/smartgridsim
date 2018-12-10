#!/bin/sh

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
for i in $(seq 1 ${numTrials}); do
    if [ `expr $i % ${numCores}` -eq 1 ] && [ $i != 1 ]
    then
        echo "Waiting previous simulations to finish..."
        wait
    fi

    echo "Starting trial ${i}"
    trialOutputDir=${outputsDir}/trial${i}
    mkdir -p ${trialOutputDir}
    (./waf --cwd="${trialOutputDir}" --run "${program} \
        --numberOfSmartMeters=${numberOfSmartMeters} \
        --maxSimulationTimeInSeconds=${maxSimulationTimeInSeconds} \
        --maxElapsedClockTimeInSeconds=${maxElapsedClockTimeInSeconds}" \
        > ${trialOutputDir}/out) &

    sleep 1 # sleep between calls to avoid races in ns-3
done
wait
echo "Done."

# go back to the project directory
cd - >/dev/null

# compute the results
./results.py
