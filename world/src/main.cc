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


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 640;
	const auto kWindowHeight = 640;
	const auto kFrameTimeMS = 1000/60;

    class World {
    public:
        World() = default;
        World(const World &) = default;
        ~World() = default;

        bool stop_flag_ = false;
        Display *display_ = nullptr;
        Window window_ = 0;
        GLXContext context_ = 0;
        Atom delete_message_ = 0;

        void run() throw() {
            init_window();
            run_loop();
            exit_window();
        }

        void init_window() throw() {
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
        }

        void exit_window() throw() {
            XUnmapWindow(display_, window_);
            glXMakeCurrent(display_, None, 0);
            glXDestroyContext(display_, context_);
            // how to destroy visual_info?
            XDestroyWindow(display_, window_);
            XCloseDisplay(display_);
        }

        void run_window() throw() {
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
                        stop_loop();
                    }
                    break;

                case KeyPress:
                    auto key = XLookupKeysym(&event.xkey, 0);
                    if (key == XK_Escape) {
                        stop_loop();
                    }
                    LOG("Event:KeyPress key=" << key);
                    break;
                }
            } else {
                draw_window();
                glXSwapBuffers(display_, window_);
            }
        }

        void draw_window() throw() {
        }

        void stop_loop() throw() {
            stop_flag_ = true;
        }

        void run_loop() throw() {
            while (stop_flag_ == false) {
                agm::sleep::milliseconds(kFrameTimeMS);
                run_window();
            }
        }
    };
}

int main(
    int argc, char *argv[]
) throw() {
    (void) argc;
    (void) argv;

    agm::log::init(AGM_TARGET_NAME ".log");

    World w;
    w.run();

    return 0;
}
