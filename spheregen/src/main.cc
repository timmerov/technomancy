/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

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

we divide each cube face into a number of quads.
we subdivide the quads into two triangles.
we have a choice.
the new edge can point towards the center of the cube face.
or it can be perpendicular.
the perpendicular case makes the sphere not round.
so when writing faces we must be careful to choose correctly.
the quad is a bit squished.
the perpendicular axis is longer (and hence lower) than
the parallel axis.

to do...
- write to file.
- command line argument for number of segments.
- texture coordinates.
- normals?
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <cmath>


namespace {
    const int kNumSegmentsInt = 7;
    const double kNumSegments = kNumSegmentsInt;
    const int kVerticesPerFace = (kNumSegmentsInt+1)*(kNumSegmentsInt+1) - 4;

    class Vector3 {
    public:
        double x_;
        double y_;
        double z_;
    };

    class CubeFaces {
    public:
        int tl_;
        int tr_;
        int bl_;
        int br_;
    };

    Vector3 g_cube_vertices[8] = {
        {+1.0, +1.0, +1.0},
        {-1.0, +1.0, +1.0},
        {+1.0, -1.0, +1.0},
        {-1.0, -1.0, +1.0},
        {+1.0, +1.0, -1.0},
        {-1.0, +1.0, -1.0},
        {+1.0, -1.0, -1.0},
        {-1.0, -1.0, -1.0}
    };

    // rectangles
    CubeFaces g_cube_faces[6] = {
        {1, 0, 3, 2},
        {0, 4, 2, 6},
        {4, 5, 6, 7},
        {0, 1, 4, 5},
        {5, 1, 7, 3},
        {3, 2, 7, 6}
    };

    double g_weights[kNumSegmentsInt+1];

    void writeHeader() throw() {
        LOG("g sphere");
    }

    void writeCubeCorners() throw() {
        int idx = 0;
        for (auto i = 0; i < 8; ++i) {
            auto v = g_cube_vertices[i];
            auto r2 = v.x_*v.x_ + v.y_*v.y_ + v.z_*v.z_;
            auto den = 1.0 / std::sqrt(r2);
            v.x_ *= den;
            v.y_ *= den;
            v.z_ *= den;
            LOG("v " << v.x_ << " " << v.y_ << " " << v.z_);
            ++idx;
        }
    }

    void initWeights() throw() {
        auto pi = std::acos(-1);
        g_weights[0] = 1.0f;
        for (auto i = 1; i < kNumSegmentsInt; ++i) {
            auto a = pi*double(i)/kNumSegments/2.0 - pi/4.0;
            g_weights[i] = (1.0 - tan(a))/2.0;
            //LOG("alpha=" << a << " weight[" << i << "]=" << g_weights[i]);
        }
        g_weights[kNumSegmentsInt] = 0.0f;
    }

    void createVertices(
        int face
    ) throw() {
        int idx = 8 + kVerticesPerFace*face;
        auto cf = g_cube_faces[face];
        auto tl = g_cube_vertices[cf.tl_];
        auto tr = g_cube_vertices[cf.tr_];
        auto bl = g_cube_vertices[cf.bl_];
        auto br = g_cube_vertices[cf.br_];
        //LOG("face={" << cf.tl_ << "," << cf.tr_ << "," << cf.bl_ << "," << cf.br_ << "}");
        //LOG("tl={" << tl.x_ << "," << tl.y_ << "," << tl.z_ << "}");
        //LOG("tr={" << tr.x_ << "," << tr.y_ << "," << tr.z_ << "}");
        //LOG("bl={" << bl.x_ << "," << bl.y_ << "," << bl.z_ << "}");
        //LOG("br={" << br.x_ << "," << br.y_ << "," << br.z_ << "}");
        for (auto y = 0; y <= kNumSegmentsInt; ++y) {
            auto tf = g_weights[y];
            auto bf = 1.0 - tf;
            Vector3 l;
            l.x_ = tl.x_*tf + bl.x_*bf;
            l.y_ = tl.y_*tf + bl.y_*bf;
            l.z_ = tl.z_*tf + bl.z_*bf;
            Vector3 r;
            r.x_ = tr.x_*tf + br.x_*bf;
            r.y_ = tr.y_*tf + br.y_*bf;
            r.z_ = tr.z_*tf + br.z_*bf;
            for (auto x = 0; x <= kNumSegmentsInt; ++x) {
                auto lf = g_weights[x];
                auto rf = 1.0 - lf;
                Vector3 v;
                v.x_ = l.x_*lf + r.x_*rf;
                v.y_ = l.y_*lf + r.y_*rf;
                v.z_ = l.z_*lf + r.z_*rf;
                auto r2 = v.x_*v.x_ + v.y_*v.y_ + v.z_*v.z_;
                auto den = 1.0 / std::sqrt(r2);
                v.x_ *= den;
                v.y_ *= den;
                v.z_ *= den;
                if ((x != 0 && x != kNumSegmentsInt)
                ||  (y != 0 && y != kNumSegmentsInt)) {
                    LOG("v " << v.x_ << " " << v.y_ << " " << v.z_);
                    ++idx;
                }
            }
        }
    }

    void createAllVertices() throw() {
        for (auto face = 0; face < 6; ++face) {
            createVertices(face);
        }
    }

    void createFaces(
        int face
    ) throw() {
        // firt build a table.
        int idx = 8 + kVerticesPerFace*face;
        int table[kNumSegmentsInt+1][kNumSegmentsInt+1];
        auto cf = g_cube_faces[face];
        table[0][0] = cf.tl_;
        table[0][kNumSegmentsInt] = cf.tr_;
        table[kNumSegmentsInt][0] = cf.bl_;
        table[kNumSegmentsInt][kNumSegmentsInt] = cf.br_;
        for (auto y = 0; y <= kNumSegmentsInt; ++y) {
            for (auto x = 0; x <= kNumSegmentsInt; ++x) {
                if ((x != 0 && x != kNumSegmentsInt)
                ||  (y != 0 && y != kNumSegmentsInt)) {
                    table[y][x] = idx;
                    ++idx;
                }
                //LOG("x=" << x << " y=" << y << " idx=" << table[y][x]);

                // obj file indexes are 1 based.
                // cause it's 1984.
                ++table[y][x];
            }
        }

        // write faces from the table
        // subdivide faces so the crease goes towards the center of the cube face.
        // otherwise they sphere is less round.
        for (auto y = 0; y < kNumSegmentsInt; ++y) {
            for (auto x = 0; x < kNumSegmentsInt; ++x) {
                auto quad = (y - kNumSegmentsInt/2)*(x - kNumSegmentsInt/2);
                if (quad <= 0) {
                    LOG("f " << table[y][x] << " " << table[y+1][x] << " " << table[y][x+1]);
                    LOG("f " << table[y][x+1] << " " << table[y+1][x] << " " << table[y+1][x+1]);
                } else {
                    LOG("f " << table[y][x] << " " << table[y+1][x] << " " << table[y+1][x+1]);
                    LOG("f " << table[y][x+1] << " " << table[y][x] << " " << table[y+1][x+1]);
                }
            }
        }
    }

    void createAllFaces() throw() {
        for (auto face = 0; face < 6; ++face) {
            createFaces(face);
        }
    }
}

int main(
    int argc, char *argv[]
) throw() {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    writeHeader();
    writeCubeCorners();
    initWeights();
    createAllVertices();
    createAllFaces();

    return 0;
}
