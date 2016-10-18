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

        void copy() throw() {
            int ht = outpng_.ht_;
            int src_stride = inpng_.stride_;
            int dst_stride = outpng_.stride_;
            int bytes = outpng_.wd_ * 3;
            auto src = inpng_.data_;
            auto dst = outpng_.data_;
            for (int i = 0; i < ht; ++i) {
                memcpy(dst, src, bytes);
                src += src_stride;
                dst += dst_stride;
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
        cvt.copy();
        cvt.outpng_.write(cvt.output_filename_);
    }

    return 0;
}
