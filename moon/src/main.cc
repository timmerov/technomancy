/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
process pictures of the moon.

some things we've learned about libraw and/or this camera.
open_file(filename)
unpack()
raw2image();
imgdata.sizes.width
imgdata.sizes.height
idx = x + y * width
imgdata.image[idx][c]
where c is the color component index is:
0: red
1: green 1
2: blue
3: green 2
note: ONE of those 4 pixels will have a value.
the other THREE will be zero.
pretty sure the bayer pattern is:
     x     x+1
y  : R/0   G1/1
y+1: G2/3  B/2
where the /# is the above color component index as per above.
one assumes one is expected to interpolate rgb values for everywhere else.
the rgb component values range from about 2k to 14k.
each r g1 b g2 component has its own slightly different black value.
r and b seem to correlate very tightly.
but g1 g2 seem to be out to lunch.
they seem to need need to be scaled (after subracting black) by about 1.9.

re interpolation...
do we consider one r g1 b g2 block to be a single pixel?
or do we interpolate rgb values for every component?
probably easier.
**/

/**
the moon is about 724 pixels wide.
it's area is about 400k pixels.
the image is 6000*4000
so the moon is about 1.7% of the pixels.
count pixels from brightest to get to 1%.
above that threshold we're definitely in the moon.
90% of the pixels are black sky pixels.
count pixels from the darkest to get to 90%.
below that threshold we're definitely in the black.
average the two thresholds for moon/not-moon pixels.

or...
use joint index.
where X% of the pixels consume 1-X% of the range.
**/

#include <libraw/libraw.h>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/png.h>

#include <vector>

namespace {

//const char *kInputFilename = "/home/timmer/Pictures/2020-05-12/moon/IMG_0393.CR2";
//const char *kOutputFilename = "moon.png";

const char *kInputFilename = "/home/timmer/Pictures/2020-07-11/IMG_0480.CR2";
const char *kOutputFilename = "comet1.png";

//const char *kInputFilename = "/home/timmer/Pictures/2020-07-11/IMG_0481.CR2";
//const char *kOutputFilename = "comet2.png";

const int kZero = 1900;

unsigned short *get_pixel(
    LibRaw& raw_image,
    int x,
    int y,
    int cmp
) {
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height;
    x = std::max(x, 0);
    y = std::max(y, 0);
    x = std::min(x, wd-1);
    y = std::min(y, ht-1);
    int idx = x + wd*y;
    auto pixel = &raw_image.imgdata.image[idx][cmp];
    return pixel;
}

void interpolate1331(
    LibRaw& raw_image,
    int x,
    int y,
    int dx,
    int dy,
    int cmp
) {
    int p0 = *get_pixel(raw_image, x-3*dx, y-3*dy, cmp);
    int p1 = *get_pixel(raw_image, x-1*dx, y-1*dy, cmp);
    int p2 = *get_pixel(raw_image, x+1*dx, y+1*dy, cmp);
    int p3 = *get_pixel(raw_image, x+3*dx, y+3*dy, cmp);
    auto pixel = get_pixel(raw_image, x, y, cmp);
    *pixel = (p0 + 3*p1 + 3*p2 + p3 - 8*kZero)/8 + kZero;
}

}

int main(
    int argc, char *argv[]
) noexcept {
    const char *in_filename = kInputFilename;
    const char *out_filename = kOutputFilename;
    if (argc > 1) {
        in_filename = argv[1];
    }
    if (argc > 2) {
        out_filename = argv[2];
    }

    agm::log::init(AGM_TARGET_NAME ".log");

    LOG("Hello, World!");

    /** load image **/
    LibRaw raw_image;
    raw_image.open_file(in_filename);
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height;
    LOG("wd="<<wd);
    LOG("ht="<<ht);
    if (wd <= 0 || ht <= 0) {
        LOG("File not opened: "<<in_filename);
        return 1;
    }
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
    std::vector<int> histr(512,0);
    std::vector<int> histg1(512,0);
    std::vector<int> histb(512,0);
    std::vector<int> histg2(512,0);
    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = raw_image.imgdata.image[idx];
            //LOG(y<<": r="<<pixel[0]<<" g1="<<pixel[1]<<" b="<<pixel[2]<<" g2="<<pixel[3]);
            int r = pixel[0];
            int g1 = pixel[1];
            int b = pixel[2];
            int g2 = pixel[3];
            if (r > 0) {
                minr = std::min(minr, r);
                maxr = std::max(maxr, r);
            }
            if (g1 > 0) {
                ming = std::min(ming, g1);
                maxg = std::max(maxg, g1);
            }
            if (b > 0) {
                minb = std::min(minb, b);
                maxb = std::max(maxb, b);
            }
            if (g2 > 0) {
                ming = std::min(ming, g2);
                maxg = std::max(maxg, g2);
            }
            sumr += r;
            sumg += g1;
            sumb += b;
            sumg += g2;
            int rx = r * 512/16384;
            int base = kZero;
            float factor = 1.9;
            int g1x = ((g1 - base) * factor + base) * 512/16384;
            int bx = b * 512/16384;
            int g2x = ((g2 - base) * factor + base) * 512/16384;
            rx = std::max(std::min(rx, 511), 0);
            g1x = std::max(std::min(g1x, 511), 0);
            bx = std::max(std::min(bx, 511), 0);
            g2x = std::max(std::min(g2x, 511), 0);
            if (r > 0) {
                histr[rx] += 1;
            }
            if (g1 > 0) {
                histg1[g1x] += 1;
            }
            if (b > 0) {
                histb[bx] += 1;
            }
            if (g2 > 0) {
                histg2[g2x] += 1;
            }
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
    std::cout<<"histogram r:";
    for (int i = 0; i < 512; ++i) {
        std::cout<<" "<<histr[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g1:";
    for (int i = 0; i < 512; ++i) {
        std::cout<<" "<<histg1[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram b:";
    for (int i = 0; i < 512; ++i) {
        std::cout<<" "<<histb[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g2:";
    for (int i = 0; i < 512; ++i) {
        std::cout<<" "<<histg2[i];
    }
    std::cout<<std::endl;

    int wd2 = wd/2;
    int ht2 = ht/2;

    /** interpolate by y **/
    for (int y = 0; y < ht2; ++y) {
        for (int x = 0; x < wd2; ++x) {
            interpolate1331(raw_image, 2*x, 2*y+1, 0, 1, 0);
            interpolate1331(raw_image, 2*x+1, 2*y+1, 0, 1, 1);
            interpolate1331(raw_image, 2*x+1, 2*y, 0, 1, 2);
            interpolate1331(raw_image, 2*x, 2*y, 0, 1, 3);
        }
    }
    /** interpolate by x **/
    for (int y = 0; y < ht2; ++y) {
        for (int x = 0; x < wd2; ++x) {
            interpolate1331(raw_image, 2*x+1, 2*y, 1, 0, 0);
            interpolate1331(raw_image, 2*x+1, 2*y+1, 1, 0, 0);
            interpolate1331(raw_image, 2*x, 2*y, 1, 0, 1);
            interpolate1331(raw_image, 2*x, 2*y+1, 1, 0, 1);
            interpolate1331(raw_image, 2*x, 2*y, 1, 0, 2);
            interpolate1331(raw_image, 2*x, 2*y+1, 1, 0, 2);
            interpolate1331(raw_image, 2*x+1, 2*y, 1, 0, 3);
            interpolate1331(raw_image, 2*x+1, 2*y+1, 1, 0, 3);
        }
    }

    Png png;
    png.init(wd, ht);
    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = raw_image.imgdata.image[idx];
            /*
            int r0 = pixel0[0];
            int g1 = pixel1[1];
            int b0 = pixel3[2];
            int g2 = pixel2[3];
            */
            //const int kDivisor = 16384;
            const int kDivisor = 600;
            const int kBase = 2200;
            const int kBaseGreen = 4275;
            int r = (pixel[0] - kBase) * 256 / kDivisor;
            int g = (pixel[1] + pixel[3] - kBaseGreen) * 256 / kDivisor;
            int b = (pixel[2] - kBase) * 256 / kDivisor;
            r = std::max(0, std::min(255, r));
            g = std::max(0, std::min(255, g));
            b = std::max(0, std::min(255, b));
            idx = x * 3 + y * png.stride_;
            png.data_[idx] = r;
            png.data_[idx+1] = g;
            png.data_[idx+2] = b;
        }
    }
    png.write(out_filename);

    /** write as ppm or tiff **/
    //raw_image.imgdata.params.use_auto_wb = 0;
    //raw_image.imgdata.params.use_camera_wb = 1;
    //raw_image.imgdata.params.use_camera_matrix = 1;
    //raw_image.imgdata.params.no_auto_bright = 1;
    //raw_image.imgdata.params.green_matching = 0;
    //raw_image.imgdata.params.adjust_maximum_thr = 0;
    /*raw_image.imgdata.params.output_tiff = 0;
    raw_image.dcraw_process();
    raw_image.dcraw_ppm_tiff_writer("moon.ppm");*/
    raw_image.recycle();

    LOG("Goodbye, World!");

    return 0;
}
