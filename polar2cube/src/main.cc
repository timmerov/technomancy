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

    class Png {
    public:
        Png() = default;
        Png(const Png &) = delete;
        ~Png() throw() {
            destruct();
        }

        void destruct() {
            delete[] data_;
            data_ = nullptr;
        }

        int wd_ = 0;
        int ht_ = 0;
        int stride_ = 0;
        png_byte *data_ = nullptr;

        bool read(
            const char *filename
        ) throw() {
            bool result = true;
            std::fstream infile;
            png_byte sig[kPngSigSize];
            png_structp png = nullptr;
            png_infop info = nullptr;
            int depth = 0;
            int channels = 0;
            int color_type = 0;
            int size = 0;
            png_byte **rows = nullptr;

            destruct();
            wd_ = 0;
            ht_ = 0;
            stride_ = 0;

            /*if (result)*/ {
                infile.open(filename, std::fstream::in);
                if (infile.is_open() == false) {
                    LOG("Failed to open file \"" << filename << "\"");
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

                wd_ = png_get_image_width(png, info);
                ht_ = png_get_image_height(png, info);
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

                stride_ = wd_ * depth * channels / 8;
                stride_ = (stride_ + 31) / 32 * 32;
                size = stride_ * ht_;

                rows = new(std::nothrow) png_byte*[ht_];
                data_ = new(std::nothrow) png_byte[size];

                auto row = data_;
                for (int i = 0; i < ht_; ++i, row += stride_) {
                    rows[i] = row;
                }

                png_read_image(png, rows);
            }

            LOG("resolution=" << wd_ << "x" << ht_);
            LOG("channels=" << channels);
            LOG("bits/channel=" << depth);
            LOG("stride=" << stride_);

            if (rows && data_) {
                agm::log::bytes(rows[ht_/2], std::min(stride_,256));
            }

            // clean up
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

        void init(
            int width,
            int height
        ) throw() {
            destruct();
            wd_ = width;
            ht_ = height;
            stride_ = (3 * wd_ + 31) / 32 * 32;
            int size = stride_ * ht_;
            data_ = new(std::nothrow) png_byte[size];
        }

        bool write(
            const char *filename
        ) throw() {
            (void) filename;
            LOG("resolution=" << wd_ << "x" << ht_);
            return true;
        }
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

        int wd = (cvt.inpng_.wd_ + 3) / 4 * 3;
        int ht = cvt.inpng_.ht_;
        cvt.outpng_.init(wd, ht);
        cvt.copy();
        cvt.outpng_.write(cvt.output_filename_);
    }

    return 0;
}