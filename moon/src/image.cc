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
