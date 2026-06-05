#!/bin/bash
 
EXECUTABLE="./posix_clock"
NAME="output_2_no_thread/clock_monotonic_raw/clock_monotonic_raw"

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root"
  exit
fi

for i in $(seq -w 0 99); do
    echo "--- Run $i ---"
    "$EXECUTABLE" | tee "${NAME}_${i}.txt"
done
 
