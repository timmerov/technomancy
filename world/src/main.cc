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
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 640;
	const auto kWindowHeight = 640;
	const auto kFrameTimeMS = 1000/60;

    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        void main() {
            gl_Position = vec4(vertex_pos_in, 1.0f);
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        out mediump vec4 frag_color;
        void main() {
            frag_color = vec4(0.8f, 0.0f, 0.0f, 1.0f);
        }
    )shader_code";

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
        GLuint array_ = 0;
        GLuint vertex_ = 0;
        GLuint index_ = 0;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;

        void run() throw() {
            init_window();
            init_gl();
            run_loop();
            exit_gl();
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
                draw_gl();
                glXSwapBuffers(display_, window_);
            }
        }

        void init_gl() throw() {
            glViewport(0, 0, kWindowWidth, kWindowHeight);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_CULL_FACE);

            glGenVertexArrays(1, &array_);
            glBindVertexArray(array_);
            LOG("array=" << array_);

            GLfloat vertex_array[] = {
                +0.0f, +0.5f, +0.0f,
                -0.5f, -0.5f, +0.0f,
                +0.5f, -0.5f, +0.0f };
            glGenBuffers(1, &vertex_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_);
            glBufferData(GL_ARRAY_BUFFER, 3*3*sizeof(GLfloat), vertex_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_);

            GLushort index_array[] = {0, 1, 2};
            glGenBuffers(1, &index_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*sizeof(GLushort), index_array, GL_STATIC_DRAW);
            LOG("index=" << index_);

            vertex_shader_ = compile_shader(GL_VERTEX_SHADER, g_vertex_source);
            fragment_shader_ = compile_shader(GL_FRAGMENT_SHADER, g_fragment_source);
            LOG("vertex_shader=" << vertex_shader_ << " fragment_shader=" << fragment_shader_);

            program_ = glCreateProgram();
            glAttachShader(program_, vertex_shader_);
            glAttachShader(program_, fragment_shader_);
            glLinkProgram(program_);
        }

        GLuint compile_shader(
            GLenum type,
            const char *source
        ) throw() {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint result = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
            int len;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (len > 0) {
                auto info = new(std::nothrow) char[len+1];
                glGetShaderInfoLog(shader, len, NULL, info);
                LOG("shader=" << shader << " type=" << type << " result=" << result << " info=\"" << info << "\"");
                delete[] info;
            }

            return shader;
        }

        void exit_gl() throw() {
            glDetachShader(program_, fragment_shader_);
            glDetachShader(program_, vertex_shader_);
            glDeleteProgram(program_);
            glDeleteShader(fragment_shader_);
            glDeleteShader(vertex_shader_);
            glDeleteBuffers(1, &index_);
            glDeleteBuffers(1, &vertex_);
            glDeleteVertexArrays(1, &array_);
        }

        void draw_gl() throw() {
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glUseProgram(program_);

            glBindVertexArray(array_);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_);

            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(0);
            glBindVertexArray(0);
            glUseProgram(0);
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
