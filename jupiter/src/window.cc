/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning jupiter example.
**/

#include "render.h"
#include "window.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/master.h>
#include <aggiornamento/window.h>


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 1200/32*32;
	const auto kWindowHeight = kWindowWidth;

    class WindowImpl : public JupiterWindow, public SimpleWindow {
    public:
        WindowImpl() = default;
        virtual ~WindowImpl() = default;

        Render *render_ = nullptr;

        virtual void init() noexcept {
            render_ = Render::create();
            simple_window_init(kWindowTitle, kWindowWidth, kWindowHeight);
            render_->init(kWindowWidth, kWindowHeight);
        }

        virtual void exit() noexcept {
            simple_window_exit();
            render_->exit();
            delete render_;
        }

        virtual void run() noexcept {
            simple_window_run();
        }

        virtual void simple_window_stop() noexcept {
            agm::master::setDone();
        }

        virtual void simple_window_size(
            int width,
            int height
        ) noexcept {
            render_->resize(width, height);
        }

        virtual void simple_window_draw() noexcept {
            render_->draw();
        }
    };
}

JupiterWindow::JupiterWindow() noexcept {
}

JupiterWindow::~JupiterWindow() noexcept {
}

JupiterWindow *JupiterWindow::create() noexcept {
    auto impl = new(std::nothrow) WindowImpl;
    return impl;
}
