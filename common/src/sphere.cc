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

to do...
- texture coordinates.
- normals?
- re-use edge vertexes.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/cmd_line.h>
#include <aggiornamento/log.h>
#include <common/sphere.h>

#include <cmath>
#include <fstream>


namespace {
    class CubeFaces {
    public:
        int tl_;
        int tr_;
        int bl_;
        int br_;
    };

    sphere::Vector3 g_cube_vertexes[8] = {
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
        {1, 3, 5, 7},
        {3, 2, 7, 6}
    };
}

sphere::Gen::Gen() throw() :
    num_segments_(0),
    vertexes_per_side_(0),
    weights_(nullptr),
    table_(nullptr),
    sphere_()
{
}

sphere::Gen::~Gen() throw() {
    destruct();
}

void sphere::Gen::destruct() throw() {
    delete[] table_;
    delete[] weights_;

    weights_ = nullptr;
    table_ = nullptr;
}

void sphere::Gen::generate(
    int num_segments,
    sphere::Sphere *sphere
) throw() {
    destruct();

    num_segments_ = num_segments;
    vertexes_per_side_ = (num_segments_ + 1) * (num_segments_ + 1);
    //LOG("vertexes_per_side=" << vertexes_per_side_);

    auto faces_per_side = 2 * num_segments_ * num_segments_;
    //LOG("faces_per_side=" << faces_per_side);

    weights_ = new(std::nothrow) double[num_segments_+1];
    table_ = new(std::nothrow) int[vertexes_per_side_];

    sphere_.num_vertexes_ = 6 * vertexes_per_side_;
    sphere_.num_faces_ = 6 * faces_per_side;
    sphere_.vertex_ = new(std::nothrow) Vector3[sphere_.num_vertexes_];
    sphere_.face_ = new(std::nothrow) Face[sphere_.num_faces_];

    //LOG("num_vertexes=" << sphere_.num_vertexes_);
    //LOG("num_faces=" << sphere_.num_faces_);

    initWeights();
    createAllVertices();
    createAllSides();

    // overwrite their sphere with ours.
    delete[] sphere->vertex_;
    delete[] sphere->face_;
    sphere->num_vertexes_ = sphere_.num_vertexes_;
    sphere->num_faces_ = sphere_.num_faces_;
    sphere->vertex_ = sphere_.vertex_;
    sphere->face_ = sphere_.face_;
    sphere_.num_vertexes_ = 0;
    sphere_.num_faces_ = 0;
    sphere_.vertex_ = nullptr;
    sphere_.face_ = nullptr;
}

void sphere::Gen::initWeights() throw() {
    auto pi = std::acos(-1);
    weights_[0] = 1.0f;
    for (int i = 1; i < num_segments_; ++i) {
        auto a = pi*double(i)/double(num_segments_)/2.0 - pi/4.0;
        weights_[i] = (1.0 - tan(a))/2.0;
        //LOG("alpha=" << a << " weight[" << i << "]=" << weights_[i]);
    }
    weights_[num_segments_] = 0.0f;
}

void sphere::Gen::createAllVertices() throw() {
    for (int side = 0; side < 6; ++side) {
        createVertices(side);
    }
}

void sphere::Gen::createVertices(
    int side
) throw() {
    int idx = vertexes_per_side_ * side;
    auto cf = g_cube_faces[side];
    auto tl = g_cube_vertexes[cf.tl_];
    auto tr = g_cube_vertexes[cf.tr_];
    auto bl = g_cube_vertexes[cf.bl_];
    auto br = g_cube_vertexes[cf.br_];
    //LOG("side={" << cf.tl_ << "," << cf.tr_ << "," << cf.bl_ << "," << cf.br_ << "}");
    //LOG("tl={" << tl.x_ << "," << tl.y_ << "," << tl.z_ << "}");
    //LOG("tr={" << tr.x_ << "," << tr.y_ << "," << tr.z_ << "}");
    //LOG("bl={" << bl.x_ << "," << bl.y_ << "," << bl.z_ << "}");
    //LOG("br={" << br.x_ << "," << br.y_ << "," << br.z_ << "}");
    for (int y = 0; y <= num_segments_; ++y) {
        auto tf = weights_[y];
        auto bf = 1.0 - tf;
        Vector3 l;
        l.x_ = tl.x_*tf + bl.x_*bf;
        l.y_ = tl.y_*tf + bl.y_*bf;
        l.z_ = tl.z_*tf + bl.z_*bf;
        Vector3 r;
        r.x_ = tr.x_*tf + br.x_*bf;
        r.y_ = tr.y_*tf + br.y_*bf;
        r.z_ = tr.z_*tf + br.z_*bf;
        for (int x = 0; x <= num_segments_; ++x) {
            auto lf = weights_[x];
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
            sphere_.vertex_[idx] = v;
            ++idx;
        }
    }
}

void sphere::Gen::createAllSides() throw() {
    for (int side = 0; side < 6; ++side) {
        createSide(side);
    }
}

void sphere::Gen::createSide(
    int side
) throw() {
    // first build a table.
    int idx = vertexes_per_side_ * side;
    auto cf = g_cube_faces[side];
    auto stride = num_segments_ + 1;
    table_[0*stride + 0] = cf.tl_;
    table_[0*stride + num_segments_] = cf.tr_;
    table_[num_segments_*stride + 0] = cf.bl_;
    table_[num_segments_*stride + num_segments_] = cf.br_;
    auto tbl = table_;
    for (int y = 0; y <= num_segments_; ++y) {
        for (int x = 0; x <= num_segments_; ++x) {
            // obj file indexes are 1 based.
            // cause it's 1984.
            *tbl = idx + 1;

            /*if (side == 0) {
                LOG("x=" << x << " y=" << y << " vtx=" << tbl->vtx_ << " tex=" << tbl->tex_);
            }*/

            ++idx;
            ++tbl;
        }
    }

    // write faces from the table
    // subdivide faces so the crease goes towards the center of the cube side.
    // otherwise they sphere is less round.
    auto faces_per_side = 2 * num_segments_ * num_segments_;
    idx = faces_per_side * side;
    auto sf = &sphere_.face_[idx];
    tbl = table_;
    //LOG("side=" << side << " idx=" << idx);
    for (int y = 0; y < num_segments_; ++y) {
        for (int x = 0; x < num_segments_; ++x) {
            auto quad = (2*y - num_segments_ + 1)*(2*x - num_segments_ + 1);
            //LOG("quad=" << quad);
            if (quad < 0) {
                sf[0].a_ = tbl[0];
                sf[0].b_ = tbl[stride];
                sf[0].c_ = tbl[1];
                sf[1].a_ = tbl[1];
                sf[1].b_ = tbl[stride];
                sf[1].c_ = tbl[stride + 1];
            } else {
                sf[0].a_ = tbl[0];
                sf[0].b_ = tbl[stride];
                sf[0].c_ = tbl[stride + 1];
                sf[1].a_ = tbl[1];
                sf[1].b_ = tbl[0];
                sf[1].c_ = tbl[stride + 1];
            }

            /*if (side == 0) {
                LOG("x=" << x << " y=" << y << " face[0]={"
                    << sf[0].a_.vtx_ << "/" << sf[0].a_.tex_ << " "
                    << sf[0].b_.vtx_ << "/" << sf[0].b_.tex_ << " "
                    << sf[0].c_.vtx_ << "/" << sf[0].c_.tex_ << "}");
                LOG("x=" << x << " y=" << y << " face[1]={"
                    << sf[1].a_.vtx_ << "/" << sf[1].a_.tex_ << " "
                    << sf[1].b_.vtx_ << "/" << sf[1].b_.tex_ << " "
                    << sf[1].c_.vtx_ << "/" << sf[1].c_.tex_ << "}");
            }*/

            sf += 2;
            ++tbl;
        }
        ++tbl;
    }
}
