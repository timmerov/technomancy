/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
simple window implementation.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/window.h>

#include <GL/glx.h>
#include <X11/Xlib.h>


namespace {
    class SimpleWindowData {
    public:
        Display *display_ = nullptr;
        Window window_ = 0;
        GLXContext context_ = 0;
        Atom delete_message_ = 0;
        int width_ = 0;
        int height_ = 0;
    };
}

SimpleWindow::SimpleWindow() noexcept {
}

SimpleWindow::~SimpleWindow() noexcept {
    auto swd = (SimpleWindowData *) opaque_;
    delete swd;
}

void SimpleWindow::simpleWindowInit(
    const char *title,
    int width,
    int height
) noexcept {
    auto swd = (SimpleWindowData *) opaque_;
    if (swd == nullptr) {
        swd = new(std::nothrow) SimpleWindowData;
        opaque_ = swd;
    }

    swd->display_ = XOpenDisplay(nullptr);
    auto root_window = DefaultRootWindow(swd->display_);
    swd->window_ = XCreateSimpleWindow(
        swd->display_,
        root_window,
        0, 0, // location (ignored by gnome)
        width, height, // size
        0, // border display_rect.width (ignored)
        BlackPixel(swd->display_, 0), // border color
        WhitePixel(swd->display_, 0) // background color
    );

    // attach the gl context to the window
    GLint attributes[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
    auto visual_info = glXChooseVisual(swd->display_, 0, attributes);
    swd->context_ = glXCreateContext(swd->display_, visual_info, nullptr, GL_TRUE);
    glXMakeCurrent(swd->display_, swd->window_, swd->context_);

    // show the window
    XMapWindow(swd->display_, swd->window_);

    // set the window title. gratuitous.
    XStoreName(swd->display_, swd->window_, title);

    // select what input we handle.
    XSelectInput(swd->display_, swd->window_, ExposureMask | KeyPressMask);
    swd->delete_message_ = XInternAtom(swd->display_, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(swd->display_, swd->window_, &swd->delete_message_, 1);

    // assume we get an expose event.
    swd->width_ = -1;
    swd->height_ = -1;
}

void SimpleWindow::simpleWindowExit() noexcept {
    auto swd = (SimpleWindowData *) opaque_;

    XUnmapWindow(swd->display_, swd->window_);
    glXMakeCurrent(swd->display_, None, 0);
    glXDestroyContext(swd->display_, swd->context_);
    // how to destroy visual_info?
    XDestroyWindow(swd->display_, swd->window_);
    XCloseDisplay(swd->display_);
}

void SimpleWindow::simple_window_run() noexcept {
    auto swd = (SimpleWindowData *) opaque_;

    auto available = XPending(swd->display_);
    if (available) {
        XEvent event;
        XNextEvent(swd->display_, &event);
        switch (event.type) {
            case Expose: {
                XWindowAttributes window_attributes;
                XGetWindowAttributes(swd->display_, swd->window_, &window_attributes);
                if (window_attributes.width != swd->width_ || window_attributes.height != swd->height_) {
                    swd->width_ = window_attributes.width;
                    swd->height_ = window_attributes.height;
                    simpleWindowSize(swd->width_, swd->height_);
                }
                break;
            }

            case ClientMessage: {
                LOG("Event:ClientMessage");
                // the user hit the close box.
                if (Atom(event.xclient.data.l[0]) == swd->delete_message_) {
                    simpleWindowStop();
                }
                break;
            }

            case KeyPress: {
				KeySym key = 0;
				char buff[4] = {0};
				XLookupString(&event.xkey, buff, sizeof(buff), &key, nullptr);
				simpleWindowKeyPressed((int) key);
            break;
            }
        }
    } else {
        simpleWindowDraw();
        glXSwapBuffers(swd->display_, swd->window_);
    }
}

void SimpleWindow::simpleWindowKeyPressed(
	int symbol
) noexcept {
	if (symbol == XK_Escape) {
		simpleWindowStop();
	}
}
