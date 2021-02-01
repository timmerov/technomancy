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
        crop_black();
        interpolate();
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
        LOG("determining black...");
        /**
        the top rows and left columns of pixels are black.
        **/
        black_.r_ = determine_black(image_.r_);
        black_.g1_ = determine_black(image_.g1_);
        black_.g2_ = determine_black(image_.g2_);
        black_.b_ = determine_black(image_.b_);
        LOG("black is: "<<black_.r_<<" "<<black_.g1_<<" "<<black_.g2_<<" "<<black_.b_);
    }

    int determine_black(
        Plane &plane
    ) {
        agm::int64 sum = 0;
        /**
        the left 37 columns are black.
        the top 16 rows are black.
        row 17 is garbage.
        the rest of the pixels are the actual image.
        **/
        int top_count = 16 * plane.width_;
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < plane.width_; ++x) {
                sum += plane.get(x, y);
            }
        }

        int left_count = 37 * (plane.height_ - 16);
        for (int y = 16; y < plane.height_; ++y) {
            for (int x = 0; x < 37; ++x) {
                sum += plane.get(x, y);
            }
        }

        int count = top_count + left_count;
        int black = sum / count;
        return black;
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

    void crop_black() {
        /**
        the left 37 columns are black.
        the top 16 rows are black.
        row 17 is garbage.
        the rest of the pixels are the actual image.
        **/
        /** left, top, right, bottom **/
        image_.crop(38, 18, image_.r_.width_, image_.r_.height_);
    }

    void interpolate() {
        /**
        applying the 1331 filter is really fast for horizontal pixels.
        so we transpose while small.
        add pixels horizontally.
        tanspose again while medium sized.
        add pixels horizontally again.
        **/
        image_.transpose();
        image_.interpolate_horz_1331();
        image_.transpose();
        image_.interpolate_horz_1331();

        /**
        at this point we have alignment issues.
        every plane has 1 too many rows and 1 too many columns.
        but every plane needs to chop a different set.
        right now every plane looks like this:
        r i r i r i
        i i i i i i
        r i r i r i
        i i i i i i
        where r is a "real" pixel and i is an interpolated pixels.
        we need to make them like up with the bayer pattern.
        r g r g r g
        g b g b g b
        r g r g r g
        g b g b g b
        **/
        int wd = image_.r_.width_ - 1;
        int ht = image_.r_.height_ - 1;
        image_.r_.crop(0, 0, wd, ht);
        image_.g1_.crop(1, 0, wd+1, ht);
        image_.g2_.crop(0, 1, wd, ht+1);
        image_.b_.crop(1, 1, wd+1, ht+1);
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
        double pwr = raw_image_.imgdata.params.gamm[0];
        double ts = raw_image_.imgdata.params.gamm[1];
        int white = 0x10000;
        gamma_curve(pwr, ts, white);

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
