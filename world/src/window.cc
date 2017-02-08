/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
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
	const auto kWindowWidth = 640;
	const auto kWindowHeight = 640;

    class WindowImpl : public WorldWindow, public SimpleWindow {
    public:
        WindowImpl() = default;
        virtual ~WindowImpl() = default;

        Render *render_ = nullptr;

        virtual void init() throw() {
            render_ = Render::create();
            simple_window_init(kWindowTitle, kWindowWidth, kWindowHeight);
            render_->init(kWindowWidth, kWindowHeight);
        }

        virtual void exit() throw() {
            simple_window_exit();
            render_->exit();
            delete render_;
        }

        virtual void run() throw() {
            simple_window_run();
        }

        virtual void simple_window_stop() throw() {
            agm::master::setDone();
        }

        virtual void simple_window_size(
            int width,
            int height
        ) throw() {
            render_->resize(width, height);
        }

        virtual void simple_window_draw() throw() {
            render_->draw();
        }
    };
}

WorldWindow::WorldWindow() throw() {
}

WorldWindow::~WorldWindow() throw() {
}

WorldWindow *WorldWindow::create() throw() {
    auto impl = new(std::nothrow) WindowImpl;
    return impl;
}
