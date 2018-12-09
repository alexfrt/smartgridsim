#!/bin/sh

set -e

rm -rf outputs
rsync -azP ../smartgrid5g poc:/home/alex/workspace/ns-allinone-3.28/ns-3.28/scratch/

ssh poc <<'ENDSSH'
    cd /home/alex/workspace/ns-allinone-3.28/ns-3.28/scratch/smartgrid5g
    ./run.sh
ENDSSH

rsync -azP poc:/home/alex/workspace/ns-allinone-3.28/ns-3.28/scratch/smartgrid5g/outputs ../smartgrid5g/