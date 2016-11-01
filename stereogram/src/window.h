/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
stereogram example.
**/

class StereoWindow {
protected:
    StereoWindow() throw();
public:
    StereoWindow(const StereoWindow &) = delete;
    static StereoWindow *create() throw();
    virtual ~StereoWindow() throw();

    virtual void init() throw() = 0;
    virtual void exit() throw() = 0;
    virtual void run() throw() = 0;
};
