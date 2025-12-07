#!/bin/bash

# Get absolute path and escape it properly
PROJECT_DIR="$(pwd)"

docker run --rm -v "${PROJECT_DIR}":/project -w /project ubuntu:22.04 bash -c '
apt-get update -qq && 
apt-get install -y -qq python3-pip g++ make && 
pip3 install -q reprozip && 
make clean && 
make && 
reprozip trace ./repcrec < tests/test1.txt && 
reprozip pack repcrec.rpz &&
echo "Done! Check repcrec.rpz"
'