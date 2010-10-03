#!/bin/bash
# password to tablet PC is vsloop

cd ../python
make clean; make
cd ../tabletpc
cp ../src/common/codes.h codes.h
perl -pi -e 's/#define //g;' codes.h
perl -pi -e 's/#ifndef//g;' codes.h
perl -pi -e 's/#endif//g;' codes.h
perl -pi -e 's/_CODES_H__//g;' codes.h
perl -pi -e 's/^(.+)[\t ]+(.+)$/$1=$2/g;' codes.h
echo 'import sys' >> codes.py
echo 'import os' >> codes.py
cat codes.h >> codes.py
rm codes.h
head codes.py
mv codes.py vsguidetablet
cp ../python/navlcm/*.py vsguidetablet
cp ../python/camlcm/*.py vsguidetablet
cp ../python/botlcm/*.py vsguidetablet
scp vsguidetablet/*.py vsguidetablet/*.glade vsguidetablet/*.png root@128.30.4.238:/home/user/vsguidetablet

