#!/bin/sh
#
# sample ./ravi-2024-07-29-2553-2578

PREFIX=$1

echo $PREFIX

./make-unix64.sh && ./build-unix64-debug/pickleball/pickleball_d -a $PREFIX.png $PREFIX+.png $PREFIX++.png
cp pickleball.log $PREFIX.txt
ffmpeg -i $PREFIX++.png $PREFIX++.jpg
