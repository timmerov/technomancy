/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
read and write png files.
wrapper for libpng.
**/

#include <png.h>


class Png {
public:
    Png() noexcept;
    Png(const Png &) = delete;
    ~Png() noexcept;

    void destruct() noexcept;

    int wd_;
    int ht_;
    int stride_;
    png_byte *data_;

    bool read(const char *filename) noexcept;

    // prep for writing.
    void init(int width, int height) noexcept;

    bool write(const char *filename) noexcept;
};
