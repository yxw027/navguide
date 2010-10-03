#!/bin/bash

for i in $(seq 1 1 50)
    do
    date >> guidance.log
    nv-guidance --map-file map-2010-01-12.13-35-15.bin -m 1
    done
