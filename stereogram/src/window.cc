/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
spinning world example.
**/

#include "render.h"
#include "window.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/master.h>
#include <aggiornamento/window.h>


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 1024;
	const auto kWindowHeight = 512;

    class WindowImpl : public StereoWindow, public SimpleWindow {
    public:
        WindowImpl() = default;
        virtual ~WindowImpl() = default;

        Render *render_ = nullptr;

        virtual void init() noexcept {
            render_ = Render::create();
            simpleWindowInit(kWindowTitle, kWindowWidth, kWindowHeight);
            render_->init(kWindowWidth, kWindowHeight);
        }

        virtual void exit() noexcept {
            simpleWindowExit();
            render_->exit();
            delete render_;
        }

        virtual void run() noexcept {
            simple_window_run();
        }

        virtual void simpleWindowStop() noexcept {
            agm::master::setDone();
        }

        virtual void simpleWindowSize(
            int width,
            int height
        ) noexcept {
            render_->resize(width, height);
        }

        virtual void simpleWindowDraw() noexcept {
            render_->draw();
        }
    };
}

StereoWindow::StereoWindow() noexcept {
}

StereoWindow::~StereoWindow() noexcept {
}

StereoWindow *StereoWindow::create() noexcept {
    auto impl = new(std::nothrow) WindowImpl;
    return impl;
}
