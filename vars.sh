#!/bin/bash

# Simulation params
numTrials=2
maxSimulationTimeInSeconds=15
maxElapsedClockTimeInSeconds=600
numbersOfSmartMeters=(100 200 300)

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
