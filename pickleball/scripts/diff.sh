#!/bin/sh
#
# sample ./pickleball/scripts/make-diff.sh /home/timmer/Pictures/2024-07-29-pickleball/ravi/frame 2553 2578 ./ravi-2024-07-29

SRC=$1
FIRST=$2
LAST=$3
DST=$4

echo $SRC $FIRST $LAST $DST

./make-unix64.sh && ./build-unix64-debug/pickleball/pickleball_d -d $SRC$FIRST.png $SRC$LAST.png $DST-$FIRST-$LAST.png
