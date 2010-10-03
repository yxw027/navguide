#!/bin/bash
#
# convert a set of WAV files to RAW using sox
#
# add pad 0 1.5 at the end of the sox command for bluetooth sound files
#
source_dir="/usr/share/sounds/"

for file in $(ls $source_dir/*.wav); do
    out=$(echo ${file##*/} | sed -e 's/.wav//g')
    echo $file" --> "$out.raw
    sox $file -c 1 -L -t raw -r 8000 $out.raw

done





