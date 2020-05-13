/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
process pictures of the moon.
**/

#include <libraw/libraw.h>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

namespace {

const char *kFilename = "/home/timmer/Pictures/2020-05-12/moon/IMG_0393.CR2";

}

int main(
    int argc, char *argv[]
) noexcept {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");

    /** load image **/
    LibRaw raw_image;
    raw_image.open_file(kFilename);
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height;
    LOG("wd="<<wd);
    LOG("ht="<<ht);
    raw_image.unpack();
    raw_image.raw2image();

    /** analyze image **/
    int minr = 99999;
    int maxr = 0;
    int ming = 99999;
    int maxg = 0;
    int minb = 99999;
    int maxb = 0;
    std::int64_t sumr = 0;
    std::int64_t sumg = 0;
    std::int64_t sumb = 0;
    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = raw_image.imgdata.image[idx];
            //LOG(y<<": r="<<pixel[0]<<" g1="<<pixel[1]<<" b="<<pixel[2]<<" g2="<<pixel[3]);
            int r = pixel[0];
            int g1 = pixel[1];
            int b = pixel[2];
            int g2 = pixel[3];
            minr = std::min(minr, r);
            maxr = std::max(maxr, r);
            ming = std::min(ming, g1);
            maxg = std::max(maxg, g1);
            minb = std::min(minb, b);
            maxb = std::max(maxb, b);
            ming = std::min(ming, g2);
            maxg = std::max(maxg, g2);
            sumr += r;
            sumg += g1;
            sumb += b;
            sumg += g2;
            if (y % 200 == 101 && x % 300 == 151) {
                int color = 2*r + g1 + g2 + 2*b - 4000;
                std::cout<<color<<" ";
            }
        }
        if (y % 200 == 101) {
            std::cout<<std::endl;
        }
    }
    LOG("minr="<<minr);
    LOG("maxr="<<maxr);
    LOG("ming="<<ming);
    LOG("maxg="<<maxg);
    LOG("minb="<<minb);
    LOG("maxb="<<maxb);
    int npixels = wd * ht;
    int avgr = sumr / npixels;
    int avgg = sumg / (npixels * 2);
    int avgb = sumb / npixels;
    LOG("avgr="<<avgr);
    LOG("avgg="<<avgg);
    LOG("avgb="<<avgb);

    /** write as tiff **/
    //raw_image.imgdata.params.use_auto_wb = 0;
    //raw_image.imgdata.params.use_camera_wb = 1;
    //raw_image.imgdata.params.use_camera_matrix = 1;
    //raw_image.imgdata.params.no_auto_bright = 1;
    //raw_image.imgdata.params.green_matching = 0;
    //raw_image.imgdata.params.adjust_maximum_thr = 0;
    raw_image.imgdata.params.output_tiff = 1;
    raw_image.dcraw_process();
    raw_image.dcraw_ppm_tiff_writer("moon.tiff");
    raw_image.recycle();

    LOG("Goodbye, World!");

    return 0;
}
