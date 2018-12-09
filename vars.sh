#!/bin/sh

# Simulation params
numTrials=30                    # central limit theorem
simulationTimeInSeconds=60      # 1 minute per trial
maxWallClockTimeInSeconds=20    # 20 seconds per trial
numberOfSmartMeters=100         # 100 smart meters per trial

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
