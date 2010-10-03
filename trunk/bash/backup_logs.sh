#!/bin/bash
#

if [ ! -n "$1" ]
then
  echo "Usage: `basename $0` target_dir"
  exit $E_BADARGS
fi

read -p "Save logs to $1? [y/N] "

if [ "$REPLY" != "y" ]
then
    exit
fi

echo "Saving logs to $1..."

for file in "../data/"/*.log
do
  target="$1/"${file##*/} 
  if [ ! -e $target ]
  then
    echo "$file --> $target"
    cp $file $target
  fi
done

