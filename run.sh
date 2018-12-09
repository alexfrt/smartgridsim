#!/bin/sh

set -e

# read the simulation variables
. ./vars.sh

# clean the outputs directory
rm -rf ${outputsDir}
mkdir -p ${outputsDir}

# go to the ns-3 directory
cd ../.. >/dev/null

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
    (./waf --cwd="${trialOutputDir}" --run ${program} > ${trialOutputDir}/out) &

    sleep 1
done
wait

# go back to the project directory
cd - >/dev/null

echo "Done."
