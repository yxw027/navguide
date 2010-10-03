#!/bin/bash
#
# convert a set of png images to an avi movie
#
# to play the movie on windows, you'll have to convert the 
# file using VirtualDub and set the compression codec to 
# Microsoft Video 1.
#

source_dir="."

# crop
echo "cropping..."
mogrify -crop 585x381+0+0 *.png

# resize
echo "resizing..."
mogrify -resize 320x210! *.png

# convert to jpg
echo "converting to jpg..."
for file in $(ls $source_dir/*.png); do
    out=$(echo ${file##*/} | sed -e 's/.png//g')
    echo $file" --> "$out.jpg
    convert $file $out.jpg

done

# make movie
echo "converting to avi..."
mencoder -ovc lavc -lavcopts vcodec=msmpeg4 -mf type=jpg:fps=8 -o movie.avi mf://\*.jpg





