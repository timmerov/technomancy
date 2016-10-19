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
- texture coordinates.
- normals?
- re-use edge vertexes.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/cmd_line.h>
#include <aggiornamento/log.h>

#include <cmath>
#include <fstream>


namespace {
    const char kDefaultFilename[] = "sphere.obj";

    const int kDefaultNumSegments = 3;

    class Vector3 {
    public:
        double x_;
        double y_;
        double z_;
    };

    class VertexTexture {
    public:
        int vtx_;
        int tex_;
    };

    class Face {
    public:
        VertexTexture a_;
        VertexTexture b_;
        VertexTexture c_;
    };

    class Sphere {
    public:
        Sphere() = default;
        ~Sphere() {
            delete[] vertex_;
            delete[] face_;
        }

        int num_vertexes_ = 0;
        int num_faces_ = 0;
        Vector3 *vertex_ = nullptr;
        Face *face_ = nullptr;
    };

    class CubeFaces {
    public:
        int tl_;
        int tr_;
        int bl_;
        int br_;
    };

    Vector3 g_cube_vertexes[8] = {
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

    class SphereGen {
    public:
        SphereGen() = default;

        ~SphereGen() throw() {
            destruct();
        }

        void destruct() throw() {
            file_.close();
            delete[] table_;
            delete[] weights_;

            weights_ = nullptr;
            table_ = nullptr;
        }

        int num_segments_ = kDefaultNumSegments;
        int textures_per_face_ = 0;
        int vertexes_per_face_ = 0;
        const char *filename_ = kDefaultFilename;
        std::fstream file_;
        double *weights_ = nullptr;
        VertexTexture *table_ = nullptr;
        Sphere sphere_;

        bool parseOptions(
            int argc,
            char *argv[]
        ) throw() {
            bool result = true;

            agm::CmdLineOptions::LongFormat cmd_line_options[] = {
                {"help",         '?'},
                {"num-segments", 'n'},
                {"output-file",  'o'},
                {nullptr, 0}
            };
            agm::CmdLineOptions clo(argc, argv, "?n:o:", cmd_line_options);
            while (clo.get()) {
                switch (clo.option_) {
                case '?':
                    showHelp();
                    break;

                case 'n':
                    num_segments_ = std::atoi(clo.value_);
                    break;

                case 'o':
                    filename_ = clo.value_;
                    break;
                }
            }
            if (clo.error_) {
                result = false;
            }

            LOG("num_segments=" << num_segments_);
            LOG("output filename=\"" << filename_ << "\"");

            return result;
        }

        void showHelp() throw() {
            LOG("Usage: spheregen [options]");
            LOG("  --help         -?  show this message");
            LOG("  --num-segments -n  subdivisions per face");
            LOG("  --output-file  -o  output file");
        }

        void generate() throw() {
            destruct();

            textures_per_face_ = (num_segments_ + 1) * (num_segments_ + 1);
            vertexes_per_face_ = textures_per_face_ - 4;
            weights_ = new(std::nothrow) double[num_segments_+1];
            table_ = new(std::nothrow) VertexTexture[textures_per_face_];

            sphere_.num_vertexes_ = 8 + 6*vertexes_per_face_;
            sphere_.num_faces_ = 6 * 2 * num_segments_ * num_segments_;
            sphere_.vertex_ = new(std::nothrow) Vector3[sphere_.num_vertexes_];
            sphere_.face_ = new(std::nothrow) Face[sphere_.num_faces_];

            //LOG("num_vertexes=" << sphere_.num_vertexes_);
            //LOG("num_faces=" << sphere_.num_faces_);

            initCubeCorners();
            initWeights();
            createAllVertices();
            createAllFaces();
        }

        void initCubeCorners() throw() {
            int idx = 0;
            for (int i = 0; i < 8; ++i) {
                auto v = g_cube_vertexes[i];
                auto r2 = v.x_*v.x_ + v.y_*v.y_ + v.z_*v.z_;
                auto den = 1.0 / std::sqrt(r2);
                v.x_ *= den;
                v.y_ *= den;
                v.z_ *= den;
                sphere_.vertex_[idx] = v;
                ++idx;
            }
        }

        void initWeights() throw() {
            auto pi = std::acos(-1);
            weights_[0] = 1.0f;
            for (int i = 1; i < num_segments_; ++i) {
                auto a = pi*double(i)/double(num_segments_)/2.0 - pi/4.0;
                weights_[i] = (1.0 - tan(a))/2.0;
                //LOG("alpha=" << a << " weight[" << i << "]=" << weights_[i]);
            }
            weights_[num_segments_] = 0.0f;
        }

        void createAllVertices() throw() {
            for (int face = 0; face < 6; ++face) {
                createVertices(face);
            }
        }

        void createVertices(
            int face
        ) throw() {
            int idx = 8 + vertexes_per_face_*face;
            auto cf = g_cube_faces[face];
            auto tl = g_cube_vertexes[cf.tl_];
            auto tr = g_cube_vertexes[cf.tr_];
            auto bl = g_cube_vertexes[cf.bl_];
            auto br = g_cube_vertexes[cf.br_];
            //LOG("face={" << cf.tl_ << "," << cf.tr_ << "," << cf.bl_ << "," << cf.br_ << "}");
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
                    if ((x != 0 && x != num_segments_)
                    ||  (y != 0 && y != num_segments_)) {
                        sphere_.vertex_[idx] = v;
                        ++idx;
                    }
                }
            }
        }

        void createAllFaces() throw() {
            for (int face = 0; face < 6; ++face) {
                createFaces(face);
            }
        }

        void createFaces(
            int face
        ) throw() {
            // first build a table.
            int tex_idx = textures_per_face_ * face;
            int vtx_idx = 8 + vertexes_per_face_*face;
            auto cf = g_cube_faces[face];
            auto stride = num_segments_ + 1;
            table_[0*stride + 0].vtx_ = cf.tl_;
            table_[0*stride + num_segments_].vtx_ = cf.tr_;
            table_[num_segments_*stride + 0].vtx_ = cf.bl_;
            table_[num_segments_*stride + num_segments_].vtx_ = cf.br_;
            auto tbl = table_;
            for (int y = 0; y <= num_segments_; ++y) {
                for (int x = 0; x <= num_segments_; ++x) {
                    if ((x != 0 && x != num_segments_)
                    ||  (y != 0 && y != num_segments_)) {
                        tbl->vtx_ = vtx_idx;
                        ++vtx_idx;
                    }

                    // obj file indexes are 1 based.
                    // cause it's 1984.
                    tbl->vtx_ += 1;

                    tbl->tex_ = tex_idx;
                    ++tex_idx;

                    /*if (face == 0) {
                        LOG("x=" << x << " y=" << y << " vtx=" << tbl->vtx_ << " tex=" << tbl->tex_);
                    }*/

                    ++tbl;
                }
            }

            // write faces from the table
            // subdivide faces so the crease goes towards the center of the cube face.
            // otherwise they sphere is less round.
            auto obj_faces_per_cube_face = 2 * num_segments_ * num_segments_;
            auto idx = obj_faces_per_cube_face * face;
            auto sf = &sphere_.face_[idx];
            tbl = table_;
            //LOG("face=" << face << " idx=" << idx);
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

                    /*if (face == 0) {
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

        void write() throw() {
            file_.open(filename_, std::fstream::out);
            writeHeader();
            writeVertexes();
            writeAllTextures();
            writeFaces();
            file_.close();
        }

        void writeHeader() throw() {
            file_ << "# generated by spheregen" << std::endl;
            file_ << "# https://github.com/timmerov/technomancy" << std::endl;
            file_ << std::endl;
            file_ << "g sphere" << std::endl;
            file_ << std::endl;
        }

        void writeVertexes() throw() {
            auto v = &sphere_.vertex_[0];
            for (int i = 0; i < sphere_.num_vertexes_; ++i, ++v) {
                file_ << "v " << v->x_ << " " << v->y_ << " " << v->z_ << std::endl;
            }
            file_ << std::endl;
        }

        void writeAllTextures() throw() {
            for (int face = 0; face < 6; ++face) {
                writeTextures(face);
            }
            file_ << std::endl;
        }

        void writeTextures(
            int face
        ) throw() {
            auto fx = double(face % 3) / 3.0;
            auto fy = double(face / 3) / 2.0;

            auto ins = 1.0 / double(num_segments_);
            for (int y = 0; y <= num_segments_; ++y) {
                auto v = fy + 1.0/2.0 * y * ins;
                for (int x = 0; x <= num_segments_; ++x) {
                    auto u = fx + 1.0/3.0 * x * ins;
                    file_ << "vt " << u << " " << v << std::endl;
                }
            }
        }

        void writeFaces() throw() {
            auto f = sphere_.face_;
            for (int i = 0; i < sphere_.num_faces_; ++i) {
                file_ << "f "
                    << f->a_.vtx_ << "/" << f->a_.tex_ << " "
                    << f->b_.vtx_ << "/" << f->b_.tex_ << " "
                    << f->c_.vtx_ << "/" << f->c_.tex_ << std::endl;
                ++f;
            }
        }
    };
}

int main(
    int argc, char *argv[]
) throw() {
    agm::log::init(AGM_TARGET_NAME ".log");

    SphereGen sphere;
    auto good = sphere.parseOptions(argc, argv);
    if (good) {
        sphere.generate();
        sphere.write();
    }

    return 0;
}
