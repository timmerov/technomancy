/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

#pragma once

/**
stereogram example.
**/

class StereoWindow {
protected:
    StereoWindow() noexcept;
public:
    StereoWindow(const StereoWindow &) = delete;
    static StereoWindow *create() noexcept;
    virtual ~StereoWindow() noexcept;

    virtual void init() noexcept = 0;
    virtual void exit() noexcept = 0;
    virtual void run() noexcept = 0;
};
