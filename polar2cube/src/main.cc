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
**/

#include "png-local.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/cmd_line.h>
#include <aggiornamento/log.h>


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
        int vtx_tl_;
        int vtx_tr_;
        int vtx_bl_;
        int vtx_br_;
    };

/*    Vector3 g_cube_vertexes[8] = {
        {+1.0, +1.0, +1.0},
        {-1.0, +1.0, +1.0},
        {+1.0, -1.0, +1.0},
        {-1.0, -1.0, +1.0},
        {+1.0, +1.0, -1.0},
        {-1.0, +1.0, -1.0},
        {+1.0, -1.0, -1.0},
        {-1.0, -1.0, -1.0}
    };*/

    // rectangles
    CubeFaces g_cube_faces[6] = {
        {1, 0, 3, 2},
        {0, 4, 2, 6},
        {4, 5, 6, 7},
        {0, 1, 4, 5},
        {5, 1, 7, 3},
        {3, 2, 7, 6}
    };

    class Convert {
    public:
        Convert() = default;
        ~Convert() = default;

        const char *input_filename_ = nullptr;
        const char *output_filename_ = nullptr;
        Png inpng_;
        Png outpng_;

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
            LOG("Usage: spheregen [options]");
            LOG("  --help        -?  show this message");
            LOG("  --input-file  -n  input file");
            LOG("  --output-file -o  output file");
        }

        void copyAllFaces() throw() {
            for (int i = 0; i < 6; ++i) {
                copyFace(i);
            }
        }

        void copyFace(
            int face
        ) throw() {
            auto f = &g_cube_faces[face];
            (void) f;

            auto wd = outpng_.wd_ / 3;
            auto ht = outpng_.ht_ / 2;
            auto x = (face % 3) * wd;
            auto y = (face / 3) * ht;

            auto r = ((face&1)>>0)*255;
            auto g = ((face&2)>>1)*255;
            auto b = ((face&4)>>2)*255;

            auto dst_row = outpng_.data_ + y*outpng_.stride_ + 3*x;
            for (int i = 0; i < ht; ++i) {
                auto dst = dst_row;
                dst_row += outpng_.stride_;
                for (int k = 0; k < wd; ++k) {
                    dst[0] = r;
                    dst[1] = g;
                    dst[2] = b;
                    dst += 3;
                }
            }
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
