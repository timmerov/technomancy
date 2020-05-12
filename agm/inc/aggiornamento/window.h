/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

#pragma once

/**
simple window interface.
**/

class SimpleWindow {
public:
    SimpleWindow() noexcept;
    SimpleWindow(const SimpleWindow &) = delete;
    virtual ~SimpleWindow() noexcept;

    // derived classes must implement the stop function.
    // the user closed the window.
    // stop the program.
    virtual void simpleWindowStop() noexcept = 0;

    // derived classes must implement the size function.
    virtual void simpleWindowSize(int width, int height) noexcept = 0;

    // derived classes must implement the draw function.
    virtual void simpleWindowDraw() noexcept = 0;

    // derived classes may override the keyPressed function.
    // symbol is an ascii character if 0..255.
    // otherwise it's an XK_ symbol.
    virtual void simpleWindowKeyPressed(int symbol) noexcept;

    // derived classes may (but probably don't need to) override these functions.
    virtual void simpleWindowInit(const char *title, int width, int height) noexcept;
    virtual void simpleWindowExit() noexcept;
    virtual void simple_window_run() noexcept;

private:
    void *opaque_ = nullptr;
};
