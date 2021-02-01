/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
data structures for image processing.
**/

#include "image.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

namespace {

int pin32bits(
    agm::int64 x
) {
    if (x < 0) {
        return 0;
    }
    if (x > 65535) {
        return 65535;
    }
    return (int) x;
}

}

Plane::Plane() {
}

Plane::~Plane() {
}

void Plane::init(
    int wd,
    int ht
) {
    width_ = wd;
    height_ = ht;
    int sz = wd * ht;
    samples_.resize(sz);
}

void Plane::set(
    int x,
    int y,
    int value
) {
    int idx = x + y * width_;
    samples_[idx] = value;
}

int Plane::get(
    int x,
    int y
) {
    int idx = x + y * width_;
    int value = samples_[idx];
    return value;
}

void Plane::scale(
    int low,
    float factor
) {
    int sz = width_ * height_;
    for (int i = 0; i < sz; ++i) {
        agm::int64 x = samples_[i];
        x = (x - low) * factor;
        samples_[i] = pin32bits(x);
    }
}

void Plane::crop(
    int left,
    int top,
    int right,
    int bottom
) {
    int new_wd = right - left;
    int new_ht = bottom - top;
    Plane dst;
    dst.init(new_wd, new_ht);
    for (int y = 0; y < new_ht; ++y) {
        for (int x = 0; x < new_wd; ++x) {
            int c = get(x + left, y + top);
            dst.set(x, y, c);
        }
    }
    *this = std::move(dst);
}

void Plane::transpose() {
    Plane dst;
    dst.init(height_, width_);
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            int c = get(x,y);
            dst.set(y, x, c);
        }
    }
    *this = std::move(dst);
}

void Plane::interpolate_horz_1331() {
    /**
    given N pixels: a b c d ... w x y z
    we are going to output: b m c m d ... w m x m y
    where n are newly interpolated pixels.
    we are going to discard the left-most and right-most pixels.
    we will have N-2 (b-y) + N-3 = 2N-5 pixels.
    **/
    int new_wd = 2*width_ - 5;
    Plane dst;
    dst.init(new_wd, height_);
    for (int y = 0; y < height_; ++y) {
        int c1 = get(0, y);
        int c2 = get(1, y);
        int c3 = get(2, y);
        int dstx = 0;
        for (int x = 3; x < width_; ++x) {
            int c0 = c1;
            c1 = c2;
            c2 = c3;
            c3 = get(x, y);
            int new_c = (c0 + 3*c1 + 3*c2 + c3) / 8;
            dst.set(dstx, y, c1);
            dst.set(dstx+1, y, new_c);
            dstx += 2;
        }
        dst.set(dstx, y, c2);
    }
    *this = std::move(dst);
}

Image::Image() {
}

Image::~Image() {
}

void Image::init(
    int wd,
    int ht
) {
    r_.init(wd, ht);
    g1_.init(wd, ht);
    g2_.init(wd, ht);
    b_.init(wd, ht);
}

void Image::crop(
    int left,
    int top,
    int right,
    int bottom
) {
    r_.crop(left, top, right, bottom);
    g1_.crop(left, top, right, bottom);
    g2_.crop(left, top, right, bottom);
    b_.crop(left, top, right, bottom);
}

void Image::transpose() {
    r_.transpose();
    g1_.transpose();
    g2_.transpose();
    b_.transpose();
}

void Image::interpolate_horz_1331() {
    r_.interpolate_horz_1331();
    g1_.interpolate_horz_1331();
    g2_.interpolate_horz_1331();
    b_.interpolate_horz_1331();
}
