#!/bin/bash

# Simulation params
numTrials=4
maxSimulationTimeInSeconds=5
maxElapsedClockTimeInSeconds=1200
numbersOfSmartMeters=(4000 2000)
aggregationPercentages=(0 25)

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
