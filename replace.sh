#!/bin/bash

version="3.35"
root="/home/grosa/Dev/ns-allinone-3.35"

echo "Copying work files..."
cp -avr "work" "${root}/ns-${version}/src"

echo "Copying data files..."
cp -avr "data" "${root}/ns-${version}"

echo "Copying simulation work file..."
cp -f "work-simulator.cc" "${root}/ns-${version}/scratch/work-simulator.cc"

echo "Copying run work file..."
cp -f "run.sh" "${root}/ns-${version}/run.sh"

echo "Building ns-3..."
cd $root
./build.py