/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
convert a polar coordinates type sphere texture to
a cube texture for use with spheregen.

the polar texture should have an aspect ratio or 2:1.
the cube texture will have an aspect ratio of 3:2.
the height of the cube texture will equal the height of the polar texture.
the cube texture width will be a multiple of 3.
the cube texture height will be a multiple of 2.

the cube has six faces: front, back, left, right, top, bottom.
the faces will be arranged in the cube texture like this:

    +--------+--------+--------+
    |        |        |        |
    | left   | front  | right  |
    |        |        |        |
    +--------+--------+--------+
    |        |        |        |
    | top    |  back  | bottom |
    |        |        |        |
    +--------+--------+--------+

the pixels in the left-front-back strip will be contiguous.
the pixels in the top-back-bottom strip will be contiguous.

the faces in the original texture look like this:

    +------------------------------------+
    |                top                 |
    +----+--------+--------+--------+----+
    |    |        |        |        |    |
    |ck  | left   | front  | right  |  ba|
    |    |        |        |        |    |
    +----+--------+--------+--------+----+
    |               bottom               |
    +------------------------------------+

the image is anti-aliased by 4x in each direction.
which makes it look somewhat blurry.
gimp filter enhance sharpen somewhere around 50 looks good.

*** caution ***
be sure to sharpen the top strip and bottom strips separately.
otherwise they'll bleed (anti-bleed?) into each other.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/cmd_line.h>
#include <aggiornamento/log.h>
#include <common/png.h>

#include <cmath>


namespace {
    class Vector3 {
    public:
        double x_;
        double y_;
        double z_;
    };

    class Coords {
    public:
        int x_;
        int y_;
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

    class Convert {
    public:
        Convert() = default;
        Convert(const Convert &) = delete;
        ~Convert() throw() {
            destruct();
        }

        void destruct() throw() {
            delete[] xweights_;
            delete[] yweights_;
            xweights_ = nullptr;
            yweights_ = nullptr;
        }

        const char *input_filename_ = nullptr;
        const char *output_filename_ = nullptr;
        Png inpng_;
        Png outpng_;
        double *xweights_ = nullptr;
        double *yweights_ = nullptr;

        bool parseOptions(
            int argc,
            char *argv[]
        ) throw() {
            bool result = true;
            bool show_help = false;

            agm::CmdLineOptions::LongFormat cmd_line_options[] = {
                {"help",        '?'},
                {"input-file",  'i'},
                {"output-file", 'o'},
                {nullptr, 0}
            };
            agm::CmdLineOptions clo(argc, argv, "?i:o:", cmd_line_options);
            while (clo.get()) {
                switch (clo.option_) {
                case '?':
                    show_help = true;
                    break;

                case 'i':
                    input_filename_ = clo.value_;
                    break;

                case 'o':
                    output_filename_ = clo.value_;
                    break;
                }
            }
            if (clo.error_) {
                result = false;
            }

            if (input_filename_ == nullptr) {
                LOG("Must specify input file.");
                show_help = true;
                result = false;
            }

            if (output_filename_ == nullptr) {
                LOG("Must specify output file.");
                show_help = true;
                result = false;
            }

            if (show_help) {
                showHelp();
            }

            LOG("input  file=\"" << input_filename_ << "\"");
            LOG("output file=\"" << output_filename_ << "\"");

            return result;
        }

        void showHelp() throw() {
            LOG("Usage: polar2cube [options]");
            LOG("  --help        -?  show this message");
            LOG("  --input-file  -i  input file");
            LOG("  --output-file -o  output file");
        }

        void copyAllFaces() throw() {
            destruct();
            auto wd = outpng_.wd_ / 3;
            auto ht = outpng_.ht_ / 2;
            xweights_ = initWeights(wd);
            yweights_ = initWeights(ht);

            for (int i = 0; i < 6; ++i) {
                copyFace(i);
            }
        }

        double *initWeights(
            int size
        ) throw() {
            // for anti-alias over-sample
            size *= 4;

            auto pi = std::acos(-1);
            auto weights = new(std::nothrow) double[size];
            for (auto i = 0; i < size; ++i) {
                auto a = pi*double(2*i+1)/double(size)/4.0 - pi/4.0;
                weights[i] = (1.0 - tan(a))/2.0;
                //LOG("alpha=" << a << " weight[" << i << "]=" << weights[i]);
            }
            return weights;
        }

        void copyFace(
            int face
        ) throw() {
            auto pi = std::acos(-1);
            auto cf = g_cube_faces[face];
            auto tl = g_cube_vertexes[cf.tl_];
            auto tr = g_cube_vertexes[cf.tr_];
            auto bl = g_cube_vertexes[cf.bl_];
            auto br = g_cube_vertexes[cf.br_];

            auto wd = outpng_.wd_ / 3;
            auto ht = outpng_.ht_ / 2;
            auto fx = (face % 3) * wd;
            auto fy = (face / 3) * ht;
            auto dst_row = outpng_.data_ + fy*outpng_.stride_ + 3*fx;

            auto aawd = 4 * wd;
            auto aaht = 4 * ht;
            auto temp_row = new(std::nothrow) int[3*wd];
            memset(temp_row, 0, sizeof(int)*3*wd);

            for (int y = 0; y < aaht; ++y) {
                int rc = 0;
                int gc = 0;
                int bc = 0;

                auto tempr = temp_row;
                auto tf = yweights_[y];
                auto bf = 1.0 - tf;
                Vector3 l;
                l.x_ = tl.x_*tf + bl.x_*bf;
                l.y_ = tl.y_*tf + bl.y_*bf;
                l.z_ = tl.z_*tf + bl.z_*bf;
                Vector3 r;
                r.x_ = tr.x_*tf + br.x_*bf;
                r.y_ = tr.y_*tf + br.y_*bf;
                r.z_ = tr.z_*tf + br.z_*bf;
                for (int x = 0; x < aawd; ++x) {
                    auto lf = xweights_[x];
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
                    auto a = atan2(-v.z_, v.x_);  // -pi (left) to +pi (right)
                    auto b = asin(v.y_);         // -pi/2 (bottom) to +pi/2 (top)
                    /*if (face == 0) {
                        if (x == 0 || y == 0) {
                            LOG("x,y=" << x << "," << y << " a,b=" << a << "," << b);
                        }
                    }*/
                    a = (a/pi + 1.0)/2.0;
                    b = 0.5 - b/pi;
                    int tx = (int) std::round(a*inpng_.wd_);
                    int ty = (int) std::round(b*inpng_.ht_);
                    auto src = inpng_.data_ + ty*inpng_.stride_ + 3*tx;
                    rc += (int)(unsigned int) src[0];
                    gc += (int)(unsigned int) src[1];
                    bc += (int)(unsigned int) src[2];

                    if ((x & 3) == 3) {
                        tempr[0] += rc;
                        tempr[1] += gc;
                        tempr[2] += bc;
                        tempr += 3;
                        rc = 0;
                        gc = 0;
                        bc = 0;
                    }
                }

                if ((y & 3) == 3) {
                    auto dst = dst_row;
                    tempr = temp_row;
                    for (auto k = 0; k < wd; ++k) {
                        dst[0] = (png_byte)((tempr[0] + 8) >> 4);
                        dst[1] = (png_byte)((tempr[1] + 8) >> 4);
                        dst[2] = (png_byte)((tempr[2] + 8) >> 4);
                        tempr[0] = 0;
                        tempr[1] = 0;
                        tempr[2] = 0;
                        dst += 3;
                        tempr += 3;
                    }
                    dst_row += outpng_.stride_;
                }
            }

            delete[] temp_row;
        }
    };
}

int main(
    int argc, char *argv[]
) throw() {
    agm::log::init(AGM_TARGET_NAME ".log");

    Convert cvt;
    auto good = cvt.parseOptions(argc, argv);
    if (good) {
        good = cvt.inpng_.read(cvt.input_filename_);

        // out width is 3/4 in width.
        // out width must be a multiple of 3.
        // out height is in height.
        // out height must be a multiple of 2.
        int wd = (cvt.inpng_.wd_ + 3) / 4 * 3;
        int ht = (cvt.inpng_.ht_ + 1) / 2 * 2;
        cvt.outpng_.init(wd, ht);
        cvt.copyAllFaces();
        cvt.outpng_.write(cvt.output_filename_);
    }

    return 0;
}
