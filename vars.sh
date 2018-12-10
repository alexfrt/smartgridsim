#!/bin/sh

# Simulation params
numTrials=8                         # central limit theorem
maxSimulationTimeInSeconds=30       # 30 seconds per trial
maxElapsedClockTimeInSeconds=600    # 10 minutes per trial
numberOfSmartMeters=100             # 100 smart meters per trial

# Execution variables
program=${PWD##*/}
outputsDir="$PWD/outputs"
numCores=$(grep -c ^processor /proc/cpuinfo)
