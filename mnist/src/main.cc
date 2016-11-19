/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
manipuate the mnist data sets.
28x28 images of handwritten digits 0..9.

open the big files.
find the first one of each class.
write to the small files.

label file format
u32 magic number 0x00000801
u32 count
u8 class value 0..9

image file format
u32 magic number 0x00000803
u32 count
u32 height = rows = 28
u32 width  = cols = 28
u32 pixels
by row
don't know if it's top to bottom or bottom to top.
0=white 255=black
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <fstream>

#include <arpa/inet.h>


namespace {
    const char kBigLabelsFilename[] = "/cb/home/timmer/github/simulate/data/mnist/trainx.raw";
    const char kBigImagesFilename[] = "/cb/home/timmer/github/simulate/data/mnist/trainy.raw";
    const char kSmallLabelsFilename[] = "/cb/home/timmer/github/simulate/net/fcnew/data10/trainx.raw";
    const char kSmallImagesFilename[] = "/cb/home/timmer/github/simulate/net/fcnew/data10/trainy.raw";

    class LabelHeader {
    public:
        agm::uint32 magic_;
        agm::uint32 count_;
    };

    class ImageHeader {
    public:
        agm::uint32 magic_;
        agm::uint32 count_;
        agm::uint32 height_;
        agm::uint32 width_;
    };
}

int main(
    int argc, char *argv[]
) throw() {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");

    std::fstream fxin;
    std::fstream fyin;
    std::fstream fxout;
    std::fstream fyout;

    fxin.open(kBigLabelsFilename, std::fstream::in);
    fyin.open(kBigImagesFilename, std::fstream::in);
    fxout.open(kSmallLabelsFilename, std::fstream::out | std::fstream::trunc);
    fyout.open(kSmallImagesFilename, std::fstream::out | std::fstream::trunc);

    LOG("fxin=" << fxin.is_open());
    LOG("fyin=" << fyin.is_open());
    LOG("fxout=" << fxout.is_open());
    LOG("fyout=" << fyout.is_open());

    fyin.seekg(0, std::ios_base::end);
    int size = (int) fyin.tellg();
    LOG("fyin.size=" << size);

    int num_in_labels = size - sizeof(LabelHeader);
    fyin.seekg(sizeof(LabelHeader), std::ios_base::beg);
    auto in_labels = new(std::nothrow) agm::uint8[num_in_labels];
    fyin.read((char *) in_labels, num_in_labels);

    ImageHeader out_image_hdr;
    out_image_hdr.magic_ = ntohl(0x00000803);
    out_image_hdr.count_ = ntohl(10);
    out_image_hdr.height_ = ntohl(28);
    out_image_hdr.width_ = ntohl(28);
    fxout.write((const char *) &out_image_hdr, sizeof(out_image_hdr));

    for (int i = 0; i < 10; ++i) {
        int idx = 0;
        for (idx = 0; idx < num_in_labels; ++idx) {
            if (in_labels[idx] == i) {
                LOG("Found " << i << " at index " << idx);
                break;
            }
        }

        fxin.seekg(sizeof(ImageHeader) + 28*28*idx);
        agm::uint8 image[28*28];
        fxin.read((char *) image, 28*28);
        fxout.write((const char *) image, 28*28);
    }

    LabelHeader out_label_hdr;
    out_label_hdr.magic_ = ntohl(0x00000801);
    out_label_hdr.count_ = ntohl(10);
    agm::uint8 out_labels[10];
    for (int i = 0; i < 10; ++i) {
        out_labels[i] = i;
    }
    fyout.write((const char *) &out_label_hdr, sizeof(out_label_hdr));
    fyout.write((const char *) out_labels, sizeof(out_labels));

    LOG("Goodbye, World!");

    return 0;
}
