/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning world example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>

#include <GL/glx.h>
#include <X11/Xlib.h>

#include "main.h"
#include "render.h"
#include "window.h"


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 640;
	const auto kWindowHeight = 640;

    class SimpleWindowImpl : public SimpleWindow {
    public:
        SimpleWindowImpl() = default;
        virtual ~SimpleWindowImpl() = default;

        Display *display_ = nullptr;
        Window window_ = 0;
        GLXContext context_ = 0;
        Atom delete_message_ = 0;
        Render *render_ = nullptr;

        virtual void init() throw() {
            display_ = XOpenDisplay(nullptr);
            auto root_window = DefaultRootWindow(display_);
            window_ = XCreateSimpleWindow(
                display_,
                root_window,
                0, 0, // location (ignored by gnome)
                kWindowWidth, kWindowHeight, // size
                0, // border display_rect.width (ignored)
                BlackPixel(display_, 0), // border color
                WhitePixel(display_, 0) // background color
            );

            // attach the gl context_ to the window_
            GLint attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
            auto visual_info = glXChooseVisual(display_, 0, attributes);
            context_ = glXCreateContext(display_, visual_info, nullptr, GL_TRUE);
            glXMakeCurrent(display_, window_, context_);

            // show the window
            XMapWindow(display_, window_);

            // set the window title. gratuitous.
            XStoreName(display_, window_, kWindowTitle);

            // select what input we handle.
            XSelectInput(display_, window_, ExposureMask | KeyPressMask);
            delete_message_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
            XSetWMProtocols(display_, window_, &delete_message_, 1);

            render_ = Render::create();
            render_->init(kWindowWidth, kWindowHeight);
        }

        virtual void exit() throw() {
            render_->exit();
            delete render_;

            XUnmapWindow(display_, window_);
            glXMakeCurrent(display_, None, 0);
            glXDestroyContext(display_, context_);
            // how to destroy visual_info?
            XDestroyWindow(display_, window_);
            XCloseDisplay(display_);
        }

        virtual void run() throw() {
            auto available = XPending(display_);
            if (available) {
                XEvent event;
                XNextEvent(display_, &event);
                switch (event.type) {
                case Expose:
                    LOG("Event:Expose");
                    XWindowAttributes window_attributes;
                    XGetWindowAttributes(display_, window_, &window_attributes);
                    //render(window_attributes.display_rect.width, window_attributes.display_rect.height);
                    glXSwapBuffers(display_, window_);
                    break;

                case ClientMessage:
                    LOG("Event:ClientMessage");
                    // the user hit the close box.
                    if (Atom(event.xclient.data.l[0]) == delete_message_) {
                        main_stop();
                    }
                    break;

                case KeyPress:
                    auto key = XLookupKeysym(&event.xkey, 0);
                    if (key == XK_Escape) {
                        main_stop();
                    }
                    LOG("Event:KeyPress key=" << key);
                    break;
                }
            } else {
                render_->draw();
                glXSwapBuffers(display_, window_);
            }
        }
    };
}

SimpleWindow::SimpleWindow() throw() {
}

SimpleWindow::~SimpleWindow() throw() {
}

SimpleWindow *SimpleWindow::create() throw() {
    auto impl = new(std::nothrow) SimpleWindowImpl;
    return impl;
}
