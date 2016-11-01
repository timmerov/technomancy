/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
simple window interface.
**/

class SimpleWindow {
public:
    SimpleWindow() throw();
    SimpleWindow(const SimpleWindow &) = delete;
    virtual ~SimpleWindow() throw();

    // derived classes must implement the stop function.
    // the user closed the window.
    // stop the program.
    virtual void simple_window_stop() throw() = 0;

    // derived classes must implement the size function.
    virtual void simple_window_size(int width, int height) throw() = 0;

    // derived classes must implement the draw function.
    virtual void simple_window_draw() throw() = 0;

    // derived classes may (but probably don't need to) override these functions.
    virtual void simple_window_init(const char *title, int width, int height) throw();
    virtual void simple_window_exit() throw();
    virtual void simple_window_run() throw();

private:
    void *opaque_ = nullptr;
};
