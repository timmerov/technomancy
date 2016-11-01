/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning world example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/simple_window.h>

#include "main.h"
#include "window.h"


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 1024;
	const auto kWindowHeight = kWindowWidth*9/16;

    class WindowImpl : public StereoWindow, public SimpleWindow {
    public:
        WindowImpl() = default;
        virtual ~WindowImpl() = default;

        virtual void init() throw() {
            simple_window_init(kWindowTitle, kWindowWidth, kWindowHeight);
        }

        virtual void exit() throw() {
            simple_window_exit();
        }

        virtual void run() throw() {
            simple_window_run();
        }

        virtual void simple_window_stop() throw() {
            main_stop();
        }

        virtual void simple_window_size(
            int width,
            int height
        ) throw() {
            (void) width;
            (void) height;
        }

        virtual void simple_window_draw() throw() {
        }
    };
}

StereoWindow::StereoWindow() throw() {
}

StereoWindow::~StereoWindow() throw() {
}

StereoWindow *StereoWindow::create() throw() {
    auto impl = new(std::nothrow) WindowImpl;
    return impl;
}
