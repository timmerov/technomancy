/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

#pragma once

/**
generate a sphere obj file.
divide a cube into a specified array of squares.
project each new vertex to the surface of the sphere.

i've chose to order the six faces thusly:
y is up: +z, +x, -z
z is up: +y, -x, -y
for the model it doesn't really matter.
but this "baseball" configuration means the first three
faces are contiguous in texture space.
i'm imagining they are the top row of a 3x2 texture.
the last three faces are similarly contiguous.

we don't divide the cube edge evenly.
the points get projected out to the surface of the sphere.
faces near the middle of the edge will stretch more.
noticably more.
so divide the edge into segments of equal angle.

we divide each cube side into a number of quads.
we subdivide the quads into two triangles.
we have a choice.
the new edge can point towards the center of the cube side.
or it can be perpendicular.
the perpendicular case makes the sphere not round.
so when writing faces we must be careful to choose correctly.
the quad is a bit squished.
the perpendicular axis is longer (and hence lower) than
the parallel axis.
**/

#include <aggiornamento/aggiornamento.h>


namespace sphere {
    class Vector2 {
    public:
        double x_;
        double y_;
    };

    class Vector3 {
    public:
        double x_;
        double y_;
        double z_;
    };

    class Face {
    public:
        int a_;
        int b_;
        int c_;
    };

    class Sphere {
    public:
        Sphere() = default;
        Sphere(const Sphere &) = delete;
        ~Sphere() {
            delete[] vertex_;
            delete[] texture_;
            delete[] face_;
        }

        int num_vertexes_ = 0;
        int num_faces_ = 0;
        Vector3 *vertex_ = nullptr;
        Vector2 *texture_ = nullptr;
        Face *face_ = nullptr;
    };

    class CubeFaces {
    public:
        int tl_;
        int tr_;
        int bl_;
        int br_;
    };

    class Gen {
    public:
        Gen() noexcept;
        Gen(const Gen &) = delete;
        ~Gen() noexcept;

        void generate(int num_segments, sphere::Sphere *sphere) noexcept;

    private:
        void destruct() noexcept;
        void initWeights() noexcept;
        void createAllVertices() noexcept;
        void createVertices(int side) noexcept;
        void createAllSides() noexcept;
        void createSide(int side) noexcept;
        void createAllTextures() noexcept;
        void createTexture(int side) noexcept;

    private:
        int num_segments_;
        int vertexes_per_side_ ;
        double *weights_;
        int *table_;
        Sphere sphere_;
    };
}
