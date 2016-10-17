/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
convert a polar coordinates type sphere texture to
a cube texture for use with spheregen.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/cmd_line.h>
#include <aggiornamento/log.h>

#include <png.h>

#include <fstream>


namespace {
    const int kPngSigSize = 8;

    class Convert {
    public:
        Convert() = default;
        ~Convert() = default;

        const char *input_filename_ = nullptr;
        const char *output_filename_ = nullptr;

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

        bool readPng() throw() {
            bool result = true;
            std::fstream infile;
            png_byte sig[kPngSigSize];
            png_structp png = nullptr;
            png_infop info = nullptr;
            int wd = 0;
            int ht = 0;
            int depth = 0;
            int channels = 0;
            int color_type = 0;
            int stride = 0;
            int size = 0;
            png_byte **rows = nullptr;
            png_byte *data = nullptr;

            /*if (result)*/ {
                infile.open(input_filename_, std::fstream::in);
                if (infile.is_open() == false) {
                    LOG("Failed to open file \"" << input_filename_ << "\"");
                    result = false;
                }
            } if (result) {
                infile.read((char *) sig, sizeof(sig));
                if (infile.good() == false) {
                    LOG("Failed to read file signature.");
                    result = false;
                }
                //agm::log::bytes(sig, kPngSigSize);
            } if (result) {
                if (png_sig_cmp(sig, 0, kPngSigSize) != 0) {
                    LOG("File signature not PNG.");
                    result = false;
                }
            } if (result) {
                png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
                if (png == nullptr) {
                    LOG("Failed to create PNG read struct.");
                        result = false;
                }
            } if (result) {
                info = png_create_info_struct(png);
                if (info == nullptr) {
                    LOG("Failed to create PNG info struct.");
                    result = false;
                }
            } if (result) {
                // in case something goes wrong in the subsequent reads.
                if (setjmp(png_jmpbuf(png))) {
                    LOG("Failed reading PNG file.");
                    result = false;
                }
            } if (result) {
                // read from the file.
                png_set_read_fn(png, (png_voidp) &infile, readData);

                png_set_sig_bytes(png, sizeof(sig));
                png_read_info(png, info);

                wd = png_get_image_width(png, info);
                ht = png_get_image_height(png, info);
                depth = png_get_bit_depth(png, info);
                channels = png_get_channels(png, info);
                color_type = png_get_color_type(png, info);

                // tell png lib we want 8 bits of r g b.
                switch (color_type) {
                case PNG_COLOR_TYPE_PALETTE:
                    png_set_palette_to_rgb(png);
                    channels = 3;
                    break;

                case PNG_COLOR_TYPE_GRAY:
                    if (depth < 8) {
                        png_set_expand_gray_1_2_4_to_8(png);
                        depth = 8;
                    }
                    break;
                }
                if (depth == 16) {
                    png_set_strip_16(png);
                }

                // in case we made any changes.
                png_read_update_info(png, info);

                stride = wd * depth * channels / 8;
                stride = (stride + 31) / 32 * 32;
                size = stride * ht;

                rows = new(std::nothrow) png_byte*[ht];
                data = new(std::nothrow) png_byte[size];

                auto row = data;
                for (int i = 0; i < ht; ++i, row += stride) {
                    rows[i] = row;
                }

                png_read_image(png, rows);
            }

            LOG("resolution=" << wd << "x" << ht);
            LOG("channels=" << channels);
            LOG("bits/channel=" << depth);
            LOG("stride=" << stride);

            if (rows && data) {
                agm::log::bytes(rows[ht/2], std::min(stride,256));
            }

            // clean up
            delete[] data;
            delete[] rows;
            if (png) {
                png_destroy_read_struct(&png, &info, nullptr);
            }

            return result;
        }

        static void readData(
            png_structp png,
            png_bytep data,
            png_size_t len
        ) throw() {
            auto f = (std::fstream *) png_get_io_ptr(png);
            f->read((char *) data, len);
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
        good = cvt.readPng();
    }

    return 0;
}
