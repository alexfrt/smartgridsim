#!/bin/sh

# Simulation params
numTrials=1
maxSimulationTimeInSeconds=3
maxElapsedClockTimeInSeconds=600
numberOfSmartMeters=30

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
