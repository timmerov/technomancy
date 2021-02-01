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
it's not clear how to get these numbers from the imgdata.
each r g1 b g2 component has its own slightly different black value.
r and b seem to correlate very tightly.
but g1 g2 seem to be out to lunch.
they seem to need need to be scaled (after subracting black) by about 1.9.

this camera seems to have some unused pixels:
the top 36 rows.
and the left 76 columns.
these numbers are hard coded in dcraw.
weird.

the height of this camera is 4051.
which is literally and figuratively odd.
should probably ignore the last row.

we should probably crop to:
left   = 76 unused + 3 interpolation + 1 round = 80
top    = 36 unused + 3 interpolation + 1 round = 40
right  = 6096 width - 3 interpolation - 1 round = 6092
bottom = 4051 height - 3 interpolation - 2 round = 4046
width  = 6092 - 80 = 6012
height = 4046 - 40 = 4006

dcraw gets the black value from the unused pixels.
that's a LOT of black pixels.
be careful of overflow.

we need to subtract black first.

second we need to do white balance and pixel scaling.
we're given cam_mul.
which gives the relative magnitudes of rgbg.
relative is the key word.
find the smallest: cam_mul_min.
range = 16384-1 - black
the scaling values are: cam_mul[i] / cam_mul_min * 65536-1 / range
multiply every pixel by the scaling values.
and now the pixel values span 0 .. 65535.
yay.

third we need to gamma correct.
we're given imgdata.params.gamm[4].
steal the gamma_curve function from dcraw.c.
dcraw does gamma correction when converting to rgb.
i don't know the values of the gamma curve.

note: if we don't care about component...
like say for to compute black.
then we can use imgdata.rawdata.raw_image.
it's the single component version of imgdata.image.

re interpolation...
currently we interpolate rgbg values for every pixel.
using a 1331 filter.
there are probably other methods.

note: there seems to be a black band top and left of some images.
perhaps there's padding in the image data.

note: there seem to be more r&b pixels than g pixels.
strange. why?

note: the base number seems to be consistent across shots.
the factor number is not.
some shots the g1 and g2 histograms are somewhat different.
it almost looks like the greens need to be offset.
maybe by the difference in the averages excluding the near blacks?

note: where are the pixels with base values?
are they near the borders?
like maybe not illuminated?
are they maybe literally camera noise?
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

#include "dump.h"
#include "image.h"

#include <libraw/libraw.h>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/png.h>

#include <cmath>
#include <vector>
#include <sstream>

namespace {

//const char *kInputFilename = "/home/timmer/Pictures/2020-05-12/moon/IMG_0393.CR2";
//const char *kOutputFilename = "moon.png";

//const char *kInputFilename = "/home/timmer/Pictures/2021-01-29/red.CR2";
//const char *kOutputFilename = "/home/timmer/Pictures/2021-01-29/red.png";

//const char *kInputFilename = "/home/timmer/Pictures/2021-01-29/green.CR2";
//const char *kOutputFilename = "/home/timmer/Pictures/2021-01-29/green.png";

//const char *kInputFilename = "/home/timmer/Pictures/2021-01-29/blue.CR2";
//const char *kOutputFilename = "/home/timmer/Pictures/2021-01-29/blue.png";

const char *kInputFilename = "/home/timmer/Pictures/2021-01-29/santa.CR2";
const char *kOutputFilename = "/home/timmer/Pictures/2021-01-29/santa.png";

//const char *kInputFilename = "/home/timmer/Pictures/2020-07-11/IMG_0480.CR2";
//const char *kOutputFilename = "comet1.png";

//const char *kInputFilename = "/home/timmer/Pictures/2020-07-11/IMG_0481.CR2";
//const char *kOutputFilename = "comet2.png";

#if 0
/**
bins in histogram.
**/
static const int kBins = 512;

/**
these hard coded numbers numbers come from pasting the histograms into a spreadsheet.
and tweaking until the rbgg graphs more/less line up.
how do we compute them automagically?
**/
/**
largest possible value recorded by camera.
**/
static const int kRange = 16384;
/**
"zero" for scaling.
**/
static const int kBase = 64.0 * kRange / kBins; // =2048
/**
kFactor scales greens by this amount.
preserve "zero".
**/
static const float kFactor = 1.9;
/**
maps to 0.
**/
static const int kZero = kBase - 3*32;
/**
maps to 255.
**/
static const int kSaturation = 441.0 * kRange / kBins;

unsigned short *get_pixel(
    LibRaw& raw_image,
    int x,
    int y,
    int cmp
) {
    /** height can be odd but shouldn't be. weird. **/
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height & ~2;
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

int scale_to_255(
    int c0,
    int c1,
    float factor
) {
    c0 = (c0 - kBase) * factor + kBase;
    c1 = (c1 - kBase) * factor + kBase;
    int c = c0 - kZero + c1 - kZero;
    c *= 255;
    c /= 2 * (kSaturation - kZero);
    c = std::max(0, std::min(c, 255));
    return c;
}

void scale_image(
    LibRaw& raw_image
) {
    /** height can be odd but shouldn't be. weird. **/
    int wd = raw_image.imgdata.sizes.width;
    int ht = raw_image.imgdata.sizes.height & ~2;
    int max = 0;

    int sz = wd * ht;
    for (int i = 0; i < sz; ++i) {
        for (int k = 0; k < 4; ++k) {
            int px = raw_image.imgdata.image[i][k];
            max = std::max(max, px);
        }
    }
    LOG("max="<<max);
    if (max > 0 && max <= 0x7FFF) {
        for (int i = 0; i < sz; ++i) {
            for (int k = 0; k < 4; ++k) {
                auto p = &raw_image.imgdata.image[i][k];
                int px = *p;
                *p = (px << 16) / max;
            }
        }
    }
}
#endif

class Black {
public:
    int r_ = 0;
    int g1_ = 0;
    int g2_ = 0;
    int b_ = 0;
};

class Balance {
public:
    float r_ = 1.0;
    float g1_ = 1.0;
    float g2_ = 1.0;
    float b_ = 1.0;
};

class Rawsome {
public:
    Rawsome() = default;
    ~Rawsome() {
        if (is_loaded_) {
            raw_image_.recycle();
        }
    }

    std::string in_filename_;
    std::string out_filename_;
    LibRaw raw_image_;
    bool is_loaded_ = false;
    Image image_;
    Black black_;
    std::vector<int> gamma_curve_;

    int run(
        const char *in_filename,
        const char *out_filename
    ) {
        in_filename_ = in_filename;
        out_filename_ = out_filename;
        load_raw_image();
        if (is_loaded_ == false) {
            return 1;
        }
        copy_raw_to_image();
        determine_black();
        scale_image();
        combine_greens();
        convert_to_srgb();
        apply_gamma();
        scale_to_8bits();
        write_png();
        return 0;
    }

    void load_raw_image() {
        LOG("loading raw image from: \""<<in_filename_<<"\"...");
        is_loaded_ = false;
        raw_image_.open_file(in_filename_.c_str());
        int raw_wd = raw_image_.imgdata.sizes.width;
        int raw_ht = raw_image_.imgdata.sizes.height;
        LOG("raw_wd="<<raw_wd);
        LOG("raw_ht="<<raw_ht);
        if (raw_wd <= 0 || raw_ht <= 0) {
            LOG("File not opened: "<<in_filename_);
            return;
        }
        raw_image_.unpack();
        raw_image_.raw2image();
        //dump(raw_image_);
        is_loaded_ = true;
    }

    void copy_raw_to_image() {
        LOG("copying raw to image...");
        /**
        raw_image_.imgdata.rawdata.raw_image is the raw samples in bayer format.
        evem rows: G B G B G B ...
        odd  rows: R G R G R G ...
        we want this pattern:
        every row: R G G B R G G B ...

        i don't know why the bayer pattern is GB/RG.
        i was expecting it to be RG/GB.
        but experimentation shows otherwise.
        maybe it has to do with the odd number of rows?
        and the pattern is bottom justified?
        dunno.
        weird.
        **/
        int raw_wd = raw_image_.imgdata.sizes.width;
        int raw_ht = raw_image_.imgdata.sizes.height;
        int wd = raw_wd / 2;
        int ht = raw_ht / 2;
        image_.init(wd, ht);

        for (int y = 0; y < ht; ++y) {
            for (int x = 0; x < wd; ++x) {
                int raw_idx = 2*x + 2*y*raw_wd;

                /** extract rggb from the GB/RG bayer pattern **/
                int g2 = raw_image_.imgdata.rawdata.raw_image[raw_idx];
                int b  = raw_image_.imgdata.rawdata.raw_image[raw_idx+1];
                int r  = raw_image_.imgdata.rawdata.raw_image[raw_idx+raw_wd];
                int g1 = raw_image_.imgdata.rawdata.raw_image[raw_idx+raw_wd+1];

                image_.r_.set(x, y, r);
                image_.g1_.set(x, y, g1);
                image_.g2_.set(x, y, g2);
                image_.b_.set(x, y, b);
            }
        }
    }

    void determine_black() {
        LOG("determining black (nyi)...");
        /**
        the top rows and left columns of pixels are black.
        **/
        /** hack **/
        black_.r_ = 2046;
        black_.g1_ = 2046;
        black_.g2_ = 2046;
        black_.b_ = 2046;
        LOG("black is: "<<black_.r_<<" "<<black_.g1_<<" "<<black_.g2_<<" "<<black_.b_);
    }

    float average_top_left(
        Plane &plane,
        int black
    ) {
        int sum = 0;
        int count = 0;
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                int c = plane.get(x, y);
                if (c > black) {
                    sum += c;
                    ++count;
                }
            }
        }
        float avg = 0.0;
        if (count) {
            avg = float(avg) / float(count);
        }
        return avg;
    }

    void scale_image() {
        LOG("scaling image...");
        /**
        combine three things:
        1. subtract black
        2. white balance
        3. scale to full 32 bits.
        **/

        /**
        normally, we would get the rggb camera multipliers from the raw_image.
        note order permutation.
        **/
        Balance cam_mul;
        auto& raw_cam_mul = raw_image_.imgdata.rawdata.color.cam_mul;
        cam_mul.r_ = raw_cam_mul[0];
        cam_mul.g1_ = raw_cam_mul[1];
        cam_mul.g2_ = raw_cam_mul[3];
        cam_mul.b_ = raw_cam_mul[2];
        //LOG("cam_mul="<<cam_mul.r_<<" "<<cam_mul.g1_<<" "<<cam_mul.g2_<<" "<<cam_mul.b_);

        /** find the smallest multiplier. **/
        float f = std::min(cam_mul.r_, cam_mul.g1_);
        f = std::min(f, cam_mul.g2_);
        f = std::min(f, cam_mul.b_);

        /** scale so smallest is 1.0 **/
        cam_mul.r_ /= f;
        cam_mul.g1_ /= f;
        cam_mul.g2_ /= f;
        cam_mul.b_ /= f;
        //LOG("cam_mul="<<cam_mul.r_<<" "<<cam_mul.g1_<<" "<<cam_mul.g2_<<" "<<cam_mul.b_);

        /**
        adjust so maximum=16383 scales to 1.0 when cam_mul is 1.0.
        (16383 - black) * cam_mul_new = 1.0 = cam_mul_org
        cam_mul_new = cam_mul_org / (16383 - black)
        **/
        cam_mul.r_ /= 16383.0 - black_.r_;
        cam_mul.g1_ /= 16383.0 - black_.g1_;
        cam_mul.g2_ /= 16383.0 - black_.g2_;
        cam_mul.b_ /= 16383.0 - black_.b_;
        //LOG("cam_mul="<<cam_mul.r_<<" "<<cam_mul.g1_<<" "<<cam_mul.g2_<<" "<<cam_mul.b_);

        /** adjust to span full 32 bit range. **/
        cam_mul.r_ *= 65535.0;
        cam_mul.g1_ *= 65535.0;
        cam_mul.g2_ *= 65535.0;
        cam_mul.b_ *= 65535.0;
        //LOG("cam_mul="<<cam_mul.r_<<" "<<cam_mul.g1_<<" "<<cam_mul.g2_<<" "<<cam_mul.b_);

        /** subtract black and white balance **/
        image_.r_.scale(black_.r_, cam_mul.r_);
        image_.g1_.scale(black_.g1_, cam_mul.g1_);
        image_.g2_.scale(black_.g2_, cam_mul.g2_);
        image_.b_.scale(black_.b_, cam_mul.b_);
    }

    void combine_greens() {
        LOG("combining greens...");
        int sz = image_.g1_.width_ * image_.g1_.height_;
        for (int i = 0; i < sz; ++i) {
            int g1 = image_.g1_.samples_[i];
            int g2 = image_.g2_.samples_[i];
            image_.g1_.samples_[i] = (g1 + g2)/2;
        }
    }

    void convert_to_srgb() {
        LOG("converting to sRGB...");
        /**
        we are currently in canon bayer rgb space.
        convert to srgb space by matrix multiplication.
        the matrix is hard-coded.
        dcraw derives it roundaboutedly.
        **/
        static double mat[3][3] = {
            {+1.901824, -0.972035, +0.070211},
            {-0.229410, +1.659384, -0.429974},
            {+0.042001, -0.519143, +1.477141}
        };
        int sz = image_.r_.width_ * image_.r_.height_;
        for (int i = 0; i < sz; ++i) {
            /** start with the original rgb. **/
            double in_r = image_.r_.samples_[i];
            double in_g = image_.g1_.samples_[i];
            double in_b = image_.b_.samples_[i];

            /** transform by matrix multiplication. **/
            double out_r = mat[0][0]*in_r + mat[0][1]*in_g + mat[0][2]*in_b;
            double out_g = mat[1][0]*in_r + mat[1][1]*in_g + mat[1][2]*in_b;
            double out_b = mat[2][0]*in_r + mat[2][1]*in_g + mat[2][2]*in_b;

            /** overwrite old values. **/
            image_.r_.samples_[i] = out_r;
            image_.g1_.samples_[i] = out_g;
            image_.b_.samples_[i] = out_b;
        }
    }

    void apply_gamma() {
        LOG("applying gamma (nyi)...");
        gamma_curve(0.45, 4.5, 0x10000);

        apply_gamma(image_.r_);
        apply_gamma(image_.g1_);
        apply_gamma(image_.b_);
    }

    /** derived from dcraw. **/
    void gamma_curve(
        double pwr,
        double ts,
        int imax
    ){
        gamma_curve_.resize(0x10000);

        #define SQR(x) ((x)*(x))
        double g2 = 0.0;
        double g3 = 0.0;
        double g4 = 0.0;

        double bnd[2] = {0.0, 0.0};
        double r;

        pwr = pwr;
        ts = ts;
        g2 = g3 = g4 = 0;
        bnd[ts >= 1] = 1;
        if ((ts-1)*(pwr-1) <= 0) {
            for (int i = 0; i < 48; ++i) {
                g2 = (bnd[0] + bnd[1])/2;
                bnd[(std::pow(g2/ts,-pwr) - 1)/pwr - 1/g2 > -1] = g2;
            }
            g3 = g2 / ts;
            g4 = g2 * (1/pwr - 1);
        }
        for (int i = 0; i < 0x10000; ++i) {
            gamma_curve_[i] = 0xffff;
            r = (double) i / imax;
            if (r < 1) {
                gamma_curve_[i] = 0x10000 * (r < g3 ? r*ts : std::pow(r,pwr)*(1+g4)-g4);
            }
        }

        /*std::stringstream ss;
        for (int i = 0; i < 0x10000; i += 0x100) {
            ss<<gamma_curve_[i]<<" ";
        }
        LOG("gamma_curve=[ "<<ss.str()<<"]");*/
    }

    void apply_gamma(
        Plane& plane
    ) {
        int sz = plane.width_ * plane.height_;
        for (int i = 0; i < sz; ++i) {
            int c = plane.samples_[i];
            c = pin_to_32bits(c);
            c = gamma_curve_[c];
            plane.samples_[i] = c;
        }
    }

    void scale_to_8bits() {
        LOG("scaling to 8 bits per sample..");
        double factor = 1.0/256.0;
        image_.r_.scale(0, factor);
        image_.g1_.scale(0, factor);
        image_.b_.scale(0, factor);
    }

    void write_png() {
        LOG("writing to: \""<<out_filename_<<"\"...");
        int wd = image_.r_.width_;
        int ht = image_.r_.height_;
        Png png;
        png.init(wd, ht);
        for (int y = 0; y < ht; ++y) {
            for (int x = 0; x < wd; ++x) {
                int r = image_.r_.get(x, y);
                int g = image_.g1_.get(x, y);
                int b = image_.b_.get(x, y);
                int idx = 3*x + y*png.stride_;
                png.data_[idx] = pin_to_8bits(r);
                png.data_[idx+1] = pin_to_8bits(g);
                png.data_[idx+2] = pin_to_8bits(b);
            }
        }
        png.write(out_filename_.c_str());
    }

    int pin_to_8bits(
        int x
    ) {
        if (x < 0) {
            return 0;
        }
        if (x > 255) {
            return 255;
        }
        return x;
    }

    int pin_to_32bits(
        int x
    ) {
        if (x < 0) {
            return 0;
        }
        if (x > 65535) {
            return 65535;
        }
        return x;
    }
};

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

    Rawsome rawsome;
    int exit_code = rawsome.run(in_filename, out_filename);

    LOG("Goodbye, World!");

    return exit_code;
}

#if 0
    /** analyze image **/
    int minr = 99999;
    int maxr = 0;
    int minb = 99999;
    int maxb = 0;
    int ming1 = 99999;
    int maxg1 = 0;
    int ming2 = 99999;
    int maxg2 = 0;
    std::int64_t sumr = 0;
    std::int64_t sumb = 0;
    std::int64_t sumg1 = 0;
    std::int64_t sumg2 = 0;
    int npixelsr = 0;
    int npixelsb = 0;
    int npixelsg1 = 0;
    int npixelsg2 = 0;
    std::vector<int> histr(kBins,0);
    std::vector<int> histb(kBins,0);
    std::vector<int> histg1(kBins,0);
    std::vector<int> histg2(kBins,0);
    std::vector<int> histg3(kBins,0);
    std::vector<int> histg4(kBins,0);

    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = raw_image.imgdata.image[idx];
            //LOG(y<<": r="<<pixel[0]<<" g1="<<pixel[1]<<" b="<<pixel[2]<<" g2="<<pixel[3]);
            int r = pixel[0];
            int b = pixel[2];
            int g1 = pixel[1];
            int g2 = pixel[3];
            int g3 = 0;
            int g4 = 0;
            if (r > 0) {
                minr = std::min(minr, r);
                maxr = std::max(maxr, r);
            }
            if (b > 0) {
                minb = std::min(minb, b);
                maxb = std::max(maxb, b);
            }
            if (g1 > 0) {
                ming1 = std::min(ming1, g1);
                maxg1 = std::max(maxg1, g1);
                g3 = (g1 - kBase) * kFactor + kBase;
            }
            if (g2 > 0) {
                ming2 = std::min(ming2, g2);
                maxg2 = std::max(maxg2, g2);
                g4 = (g2 - kBase) * kFactor + kBase;
            }
            sumr += r;
            sumb += b;
            sumg1 += g1;
            sumg2 += g2;
            int rx = r * kBins/kRange;
            int bx = b * kBins/kRange;
            int g1x = g1 * kBins/kRange;
            int g2x = g2 * kBins/kRange;
            int g3x = g3 * kBins/kRange;
            int g4x = g4 * kBins/kRange;
            rx = std::max(std::min(rx, kBins-1), 0);
            bx = std::max(std::min(bx, kBins-1), 0);
            g1x = std::max(std::min(g1x, kBins-1), 0);
            g2x = std::max(std::min(g2x, kBins-1), 0);
            g3x = std::max(std::min(g3x, kBins-1), 0);
            g4x = std::max(std::min(g4x, kBins-1), 0);
            if (r > 0) {
                histr[rx] += 1;
                ++npixelsr;
            }
            if (b > 0) {
                histb[bx] += 1;
                ++npixelsb;
            }
            if (g1 > 0) {
                histg1[g1x] += 1;
                ++npixelsg1;
            }
            if (g2 > 0) {
                histg2[g2x] += 1;
                ++npixelsg2;
            }
            if (g3 > 0) {
                histg3[g3x] += 1;
            }
            if (g4 > 0) {
                histg4[g4x] += 1;
            }
        }
    }
    int avgr = sumr / npixelsr;
    int avgb = sumb / npixelsb;
    int avgg1 = sumg1 / npixelsg1;
    int avgg2 = sumg2 / npixelsg2;
    LOG("npixels r/b/g1/g2="<<npixelsr<<"/"<<npixelsb<<"/"<<npixelsg1<<"/"<<npixelsg2);
    LOG("min/avg/max r="<<minr<<"/"<<avgr<<"/"<<maxr);
    LOG("min/avg/max b="<<minb<<"/"<<avgb<<"/"<<maxb);
    LOG("min/avg/max g1="<<ming1<<"/"<<avgg1<<"/"<<maxg1);
    LOG("min/avg/max g2="<<ming2<<"/"<<avgg2<<"/"<<maxg2);
    std::cout<<"histogram r:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histr[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram b:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histb[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g1:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg1[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g2:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg2[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g3:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg3[i];
    }
    std::cout<<std::endl;
    std::cout<<"histogram g4:";
    for (int i = 0; i < kBins; ++i) {
        std::cout<<" "<<histg4[i];
    }
    std::cout<<std::endl;

    int width = 0;
    int height = 0;
    int colors = 0;
    int bps = 0;
    raw_image.get_mem_image_format(&width, &height, &colors, &bps);
    LOG("image format width="<<width<<" height="<<height<<" colors="<<colors<<" bps="<<bps);

    //int x = wd/2/2*2;
    //int y = ht/2/2*2;
    //int x = 4336;
    //int y = 3076;
    //int x = 2748;
    //int y = 2854;
    //int x = 76-2;
    //int y = 34-2;
    int x = 2;
    int y = 2;
    LOG("center pixels:");
    int p0 = *get_pixel(raw_image, x, y, 0);
    int p1 = *get_pixel(raw_image, x, y, 1);
    int p2 = *get_pixel(raw_image, x, y, 2);
    int p3 = *get_pixel(raw_image, x, y, 3);
    LOG("x,y="<<x<<","<<y<<" pixel="<<p0<<","<<p1<<","<<p2<<","<<p3);
    ++x;
    p0 = *get_pixel(raw_image, x, y, 0);
    p1 = *get_pixel(raw_image, x, y, 1);
    p2 = *get_pixel(raw_image, x, y, 2);
    p3 = *get_pixel(raw_image, x, y, 3);
    LOG("x,y="<<x<<","<<y<<" pixel="<<p0<<","<<p1<<","<<p2<<","<<p3);
    --x;
    ++y;
    p0 = *get_pixel(raw_image, x, y, 0);
    p1 = *get_pixel(raw_image, x, y, 1);
    p2 = *get_pixel(raw_image, x, y, 2);
    p3 = *get_pixel(raw_image, x, y, 3);
    LOG("x,y="<<x<<","<<y<<" pixel="<<p0<<","<<p1<<","<<p2<<","<<p3);
    ++x;
    p0 = *get_pixel(raw_image, x, y, 0);
    p1 = *get_pixel(raw_image, x, y, 1);
    p2 = *get_pixel(raw_image, x, y, 2);
    p3 = *get_pixel(raw_image, x, y, 3);
    LOG("x,y="<<x<<","<<y<<" pixel="<<p0<<","<<p1<<","<<p2<<","<<p3);

    --x;
    --y;
    int c0 = raw_image.COLOR(y, x);
    int c1 = raw_image.COLOR(y, x+1);
    int c2 = raw_image.COLOR(y+1, x);
    int c3 = raw_image.COLOR(y+1, x+1);
    LOG("COLOR="<<c0<<","<<c1<<","<<c2<<","<<c3);

    int fc0 = raw_image.FC(y, x);
    int fc1 = raw_image.FC(y, x+1);
    int fc2 = raw_image.FC(y+1, x);
    int fc3 = raw_image.FC(y+1, x+1);
    LOG("FC="<<fc0<<","<<fc1<<","<<fc2<<","<<fc3);

    int fcol0 = raw_image.fcol(y, x);
    int fcol1 = raw_image.fcol(y, x+1);
    int fcol2 = raw_image.fcol(y+1, x);
    int fcol3 = raw_image.fcol(y+1, x+1);
    LOG("fcol="<<fcol0<<","<<fcol1<<","<<fcol2<<","<<fcol3);


#if 0
    /**
    this test code ensures component values are where we think they are.
    even rows: RR G1 RR G1 RR G1 RR G1 RR G1 RR G1 RR G1 RR G1 RR G1 RR G1
    odd  rows: G2 BB G2 BB G2 BB G2 BB G2 BB G2 BB G2 BB G2 BB G2 BB G2 BB
    all other components are 0.
    the index for each component is: RR=0 G1=1 BB=2 G2=3.
    **/

    int unexpected0 = 0;
    int unexpected1 = 0;
    int unexpected2 = 0;
    int unexpected3 = 0;
    for (int y = 0; y < ht; y += 2) {
        for (int x = 0; x < wd; x += 2) {
            auto p = &raw_image.imgdata.image[x + wd*y][0];
            if (p[1] || p[2] || p[3]) {
                ++unexpected0;
                LOG("unexpected0: x,y="<<x<<","<<y<<" p="<<p[0]<<","<<p[1]<<","<<p[2]<<","<<p[3]);
            }
            p += 4;
            if (p[0] || p[2] || p[3]) {
                ++unexpected1;
                LOG("unexpected1: x,y="<<x<<","<<y<<" p="<<p[0]<<","<<p[1]<<","<<p[2]<<","<<p[3]);
            }
            p += 4*(wd - 1);
            if (p[0] || p[1] || p[2]) {
                ++unexpected2;
                LOG("unexpected2: x,y="<<x<<","<<y<<" p="<<p[0]<<","<<p[1]<<","<<p[2]<<","<<p[3]);
            }
            p += 4;
            if (p[0] || p[1] || p[3]) {
                ++unexpected3;
                LOG("unexpected3: x,y="<<x<<","<<y<<" p="<<p[0]<<","<<p[1]<<","<<p[2]<<","<<p[3]);
            }
        }
    }
    LOG("unexpected0="<<unexpected0);
    LOG("unexpected1="<<unexpected1);
    LOG("unexpected2="<<unexpected2);
    LOG("unexpected3="<<unexpected3);
#endif

#if 1
#if 0
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
#else
    (void) interpolate1331;
#endif

    Png png;
    png.init(wd, ht);
    for (int y = 0; y < ht; ++y) {
        for (int x = 0; x < wd; ++x) {
            int idx = x + y*wd;
            auto pixel = &raw_image.imgdata.image[idx][0];
            int r0 = pixel[0];
            int b0 = pixel[2];
            int g1 = pixel[1];
            int g2 = g1; //pixel[3];
            /*
            int r = scale_to_255(r0, r0, 1.0);
            int b = scale_to_255(b0, b0, 1.0);
            int g = scale_to_255(g1, g2, kFactor);
            */
            (void) scale_to_255;
            int r = r0 / 256;
            int b = b0 / 256;
            int g = (g1 + g2) / 256;
            idx = x * 3 + y * png.stride_;
            png.data_[idx] = r;
            png.data_[idx+1] = g;
            png.data_[idx+2] = b;
        }
    }
    LOG("writing to "<<out_filename);
    png.write(out_filename);
#else
    (void) out_filename;
#endif

    raw_image.recycle();

    return 0;
}
#endif
