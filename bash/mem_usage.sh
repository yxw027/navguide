#!/bin/bash
#

if [ ! -n "$1" ]
then
    echo "Usage: $0 <process_name>"
    exit $E_BADARGS
fi

name=$1

# gnuplot file
ps aux | grep $name | grep -v grep | grep -v $0 >> mem_usage.txt
echo "plot 'mem_usage.txt' using 5; pause 1; reread;" > mem_usage.gnu
gnuplot mem_usage.gnu &

echo "Tracking process "$name". Writing output to mem_usage.txt. Use gnuplot".

while [ "0" -lt "1" ]
do
    sleep 1
    ps aux | grep $name | grep -v grep | grep -v $0 >> mem_usage.txt
done
