#!/bin/sh

set -e

program=${PWD##*/}
outputs="$PWD/outputs"

mkdir -p ${outputs}
rm -rf ${outputs}/*

cd ../.. >/dev/null
./waf --cwd="${outputs}" --run ${program}
cd - >/dev/null
