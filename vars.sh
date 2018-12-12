#!/bin/bash

# Simulation params
numTrials=2
maxSimulationTimeInSeconds=15
maxElapsedClockTimeInSeconds=1500
numbersOfSmartMeters=(1600 800 400 200)

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
