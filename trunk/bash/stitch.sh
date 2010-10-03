#!/bin/bash
#
#
echo "processing image "$1
convert +append $1"-1.png" $1"-2.png" top.png
convert +append $1"-3.png" $1"-0.png" bottom.png
convert -append top.png bottom.png $1"-stitch.png"
rm bottom.png
rm top.png
echo "output written to "$1"-stitch.png"


