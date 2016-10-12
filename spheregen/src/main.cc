/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
generate a sphere obj file.
divide a cube into a specified array of squares.
project each new vertex to the surface of the sphere.

some segments will be longer than others as measured
by the great circle.
but not much.
so oh well.

i've chose to order the six faces thusly:
y is up: +z, +x, -z
z is up: +y, -x, -y
for the model it doesn't really matter.
but this "baseball" configuration means the first three
faces are contiguous in texture space.
i'm imagining they are the top row of a 3x2 texture.
the last three faces are similarly contiguous.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>


namespace {
    class Vector3 {
    public:
        double x_;
        double y_;
        double z_;
    };

    // rectangles
    Vector3 g_cube_faces[] = {
        {-1.0, +1.0, +1.0}, {+1.0, +1.0, +1.0}, {+1.0, -1.0, +1.0}, {-1.0, -1.0, +1.0},
        {+1.0, +1.0, +1.0}, {+1.0, +1.0, -1.0}, {+1.0, -1.0, -1.0}, {+1.0, -1.0, +1.0},
        {+1.0, +1.0, -1.0}, {-1.0, +1.0, -1.0}, {-1.0, -1.0, -1.0}, {+1.0, -1.0, -1.0},

        {+1.0, +1.0, +1.0}, {-1.0, +1.0, +1.0}, {-1.0, +1.0, -1.0}, {+1.0, +1.0, -1.0},
        {-1.0, +1.0, +1.0}, {-1.0, -1.0, +1.0}, {-1.0, -1.0, -1.0}, {-1.0, +1.0, -1.0},
        {-1.0, -1.0, +1.0}, {+1.0, -1.0, +1.0}, {+1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0}
    };
}

int main(
    int argc, char *argv[]
) throw() {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");
    LOG("Goodbye, World!");

    return 0;
}
