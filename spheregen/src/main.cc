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

    class Sphere {
    public:
        Sphere() = default;
        ~Sphere() {
            delete[] vertex_;
            delete[] face_;
        }

        Vector3 *vertex_ = nullptr;
        int *face_ = nullptr;
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

    class SphereGen {
    public:
        SphereGen() = default;

        ~SphereGen() throw() {
            file_.close();
            delete[] weights_;
        }

        int num_segments_ = kDefaultNumSegments;
        int vertices_per_face_ = 0;
        const char *filename_ = kDefaultFilename;
        std::fstream file_;
        double *weights_ = nullptr;
        Sphere sphere_;

        void generate(
            int argc,
            char *argv[]
        ) throw() {
            bool result = parseOptions(argc, argv);
            if (result == false) {
                return;
            }

            vertices_per_face_ = (num_segments_ + 1)*(num_segments_ + 1) - 4;
            weights_ = new(std::nothrow) double[num_segments_+1];

            auto num_vertices = 8 + 6*vertices_per_face_;
            auto num_faces = 3 * num_segments_ * num_segments_;
            sphere_.vertex_ = new(std::nothrow) Vector3[num_vertices];
            sphere_.face_ = new(std::nothrow) int[3*num_faces];

            openFile();
            writeHeader();
            writeCubeCorners();
            initWeights();
            createAllVertices();
            createAllFaces();
        }

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

        void openFile() throw() {
            file_.open(filename_, std::fstream::out);
        }

        void writeHeader() throw() {
            file_ << "# generated by spheregen" << std::endl;
            file_ << "# https://github.com/timmerov/technomancy" << std::endl;
            file_ << std::endl;
            file_ << "g sphere" << std::endl;
            file_ << std::endl;
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
                file_ << "v " << v.x_ << " " << v.y_ << " " << v.z_ << std::endl;
                sphere_.vertex_[idx] = v;
                ++idx;
            }
        }

        void initWeights() throw() {
            auto pi = std::acos(-1);
            weights_[0] = 1.0f;
            for (auto i = 1; i < num_segments_; ++i) {
                auto a = pi*double(i)/double(num_segments_)/2.0 - pi/4.0;
                weights_[i] = (1.0 - tan(a))/2.0;
                //LOG("alpha=" << a << " weight[" << i << "]=" << weights_[i]);
            }
            weights_[num_segments_] = 0.0f;
        }

        void createAllVertices() throw() {
            for (auto face = 0; face < 6; ++face) {
                createVertices(face);
            }
            file_ << std::endl;
        }

        void createVertices(
            int face
        ) throw() {
            int idx = 8 + vertices_per_face_*face;
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
            for (auto y = 0; y <= num_segments_; ++y) {
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
                for (auto x = 0; x <= num_segments_; ++x) {
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
                        file_ << "v " << v.x_ << " " << v.y_ << " " << v.z_ << std::endl;
                        sphere_.vertex_[idx] = v;
                        ++idx;
                    }
                }
            }
        }

        void createAllFaces() throw() {
            for (auto face = 0; face < 6; ++face) {
                createFaces(face);
            }
            file_ << std::endl;
        }

        void createFaces(
            int face
        ) throw() {
            // first build a table.
            int idx = 8 + vertices_per_face_*face;
            int table[num_segments_+1][num_segments_+1];
            auto cf = g_cube_faces[face];
            table[0][0] = cf.tl_;
            table[0][num_segments_] = cf.tr_;
            table[num_segments_][0] = cf.bl_;
            table[num_segments_][num_segments_] = cf.br_;
            for (auto y = 0; y <= num_segments_; ++y) {
                for (auto x = 0; x <= num_segments_; ++x) {
                    if ((x != 0 && x != num_segments_)
                    ||  (y != 0 && y != num_segments_)) {
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
            idx = 0;
            for (auto y = 0; y < num_segments_; ++y) {
                for (auto x = 0; x < num_segments_; ++x) {
                    auto quad = (2*y - num_segments_ + 1)*(2*x - num_segments_ + 1);
                    //LOG("quad=" << quad);
                    if (quad < 0) {
                        sphere_.face_[idx+0] = table[y][x];
                        sphere_.face_[idx+1] = table[y+1][x];
                        sphere_.face_[idx+2] = table[y][x+1];
                        sphere_.face_[idx+3] = table[y][x+1];
                        sphere_.face_[idx+4] = table[y+1][x];
                        sphere_.face_[idx+5] = table[y+1][x+1];
                    } else {
                        sphere_.face_[idx+0] = table[y][x];
                        sphere_.face_[idx+1] = table[y+1][x];
                        sphere_.face_[idx+2] = table[y+1][x+1];
                        sphere_.face_[idx+3] = table[y][x+1];
                        sphere_.face_[idx+4] = table[y][x];
                        sphere_.face_[idx+5] = table[y+1][x+1];
                    }
                    file_ << "f " << sphere_.face_[idx+0] << " " << sphere_.face_[idx+1] << " " << sphere_.face_[idx+2] << std::endl;
                    file_ << "f " << sphere_.face_[idx+3] << " " << sphere_.face_[idx+4] << " " << sphere_.face_[idx+6] << std::endl;
                    idx += 6;
                }
            }
        }
    };
}

int main(
    int argc, char *argv[]
) throw() {
    agm::log::init(AGM_TARGET_NAME ".log");

    SphereGen sphere;
    sphere.generate(argc, argv);

    return 0;
}
