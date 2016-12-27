#!/bin/sh

echo "sample do everything script..."

./gen-unix.sh
./make-unix.sh

./build-unix64-debug/polar2cube/polar2cube_d -i data/polar.png -o cube.png
./build-unix64-debug/polar2cube/polar2cube_d -i data/nightearth.png -o nightcube.png
./build-unix64-debug/polar2cube/polar2cube_d -i data/jupiter.png -o jupiter-cube.png

echo "open cube.png, nightcube.png, and jupiter-cube.png with gimp."
echo "sharpen 50 the top halves and the bottom halves separately."
echo "save as cube-sharp50.png, nightcube-sharp50.png, and jupiter-cube-sharp50.png"

./build-unix64-debug/stereogram/stereogram_d
./build-unix64-debug/world/world_d
./build-unix64-debug/jupiter/jupiter_d

echo "use ffmpeg to create super high resolution videos."
ffmpeg -r 60 -f image2 -s 1184x1184 -i output/world%05d.png -vcodec libx264 -crf 20 -pix_fmt yuv420p -vf scale=1184:1184 jupiter.mp4
