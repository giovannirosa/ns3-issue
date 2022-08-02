#!/bin/bash

echo "Executing Work simulation..."
./waf --run "scratch/work-simulator --pcap" > log.txt 2>&1

    
