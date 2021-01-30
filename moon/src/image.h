/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
data structures for image processing.
**/

#include <vector>

class Plane {
public:
    Plane();
    ~Plane();

    int width_ = 0;
    int height_ = 0;
    std::vector<int> samples_;

    /** allocate space for the samples. **/
    void init(int wd, int ht);

    /** set and get the sample value at the location. **/
    void set(int x, int y, int value);
    int get(int x, int y);

    /** scale low to 0 and high to 65535. **/
    void scale(int low, int high);

    /** scale low to 0 and multiply by the factor. **/
    void scale(int low, float factor);
};

class Image {
public:
    Image();
    ~Image();

    Plane r_;
    Plane g1_;
    Plane g2_;
    Plane b_;

    /** initialize all of the planes. **/
    void init(int wd, int ht);
};
