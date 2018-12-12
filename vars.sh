#!/bin/bash

# Simulation params
numTrials=30
maxSimulationTimeInSeconds=5
maxElapsedClockTimeInSeconds=1200
numbersOfSmartMeters=(10000 8000 6000 4000 2000)

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
