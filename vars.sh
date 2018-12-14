#!/bin/bash

# Simulation params
numTrials=${NUM_TRIALS:-30}
maxSimulationTimeInSeconds=${SIM_TIME:-5}
maxElapsedClockTimeInSeconds=${WALL_TIME:-1200}
numbersOfSmartMeters=${NUM_METERS:-10000 8000 6000 4000 2000}
aggregationPercentages=${AGG_PERCENTAGES:-0 25 50}

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
