# technomancy
experiments: opengl es spinning world.

to build:

$ ./make-unix64.sh
$ ./build-unix64-debug/polar2cube/polar2cube_d -i data/polar.png -o cube.png
$ gimp cube.png

select the top half of the image.
filter -> enhance -> sharpen
sharpness 50
okay

select the bottom half of the image.
filter -> enhance -> sharpen
sharpness 50
okay

file -> export as
cube-sharp50.png

$ ./build-unix64-debug/world/world_d

build for windows is not supported.