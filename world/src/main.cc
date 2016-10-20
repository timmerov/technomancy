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
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace {
	const auto kWindowTitle = AGM_TARGET_NAME;
	const auto kWindowWidth = 640;
	const auto kWindowHeight = 640;
	const auto kFrameTimeMS = 1000/60;

    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        uniform mat4 proj_view_mat;
        void main() {
            gl_Position = proj_view_mat * vec4(vertex_pos_in, 1.0f);
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        out mediump vec4 frag_color;
        void main() {
            frag_color = vec4(0.0f, 0.8f, 0.8f, 1.0f);
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
        GLuint vertex_buffer_ = 0;
        GLuint index_buffer_ = 0;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint proj_view_mat_loc_ = 0;

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

            GLfloat vertex_array[] = {
                +0.0f, +1.0f, +0.0f,
                -1.0f, -1.0f, +0.0f,
                +1.0f, -1.0f, +0.0f };
            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, 3*3*sizeof(GLfloat), vertex_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            GLushort index_array[] = {0, 1, 2};
            glGenBuffers(1, &index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3*sizeof(GLushort), index_array, GL_STATIC_DRAW);
            LOG("index=" << index_buffer_);

            vertex_shader_ = compile_shader(GL_VERTEX_SHADER, g_vertex_source);
            LOG("vertex_shader=" << vertex_shader_);
            fragment_shader_ = compile_shader(GL_FRAGMENT_SHADER, g_fragment_source);
            LOG("fragment_shader=" << fragment_shader_);

            program_ = glCreateProgram();
            LOG("program=" << program_);
            glAttachShader(program_, vertex_shader_);
            glAttachShader(program_, fragment_shader_);
            glLinkProgram(program_);

            GLint result = GL_FALSE;
            int len;
            glGetProgramiv(program_, GL_LINK_STATUS, &result);
            glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &len);
            if (len > 0) {
                auto info = new(std::nothrow) char[len+1];
                glGetProgramInfoLog(program_, len, nullptr, info);
                LOG("error log: " << info);
                delete[] info;
            }

            glUseProgram(program_);
            proj_view_mat_loc_ = glGetUniformLocation(program_, "proj_view_mat");
            LOG("proj_view_mat_loc=" << proj_view_mat_loc_);
        }

        GLuint compile_shader(
            GLenum type,
            const char *source
        ) throw() {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            GLint result = GL_FALSE;
            int len;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (len > 0) {
                auto info = new(std::nothrow) char[len+1];
                glGetShaderInfoLog(shader, len, nullptr, info);
                LOG("error log: " << info);
                delete[] info;
            }

            return shader;
        }

        void exit_gl() throw() {
            if (program_) {
                if (fragment_shader_) {
                    glDetachShader(program_, fragment_shader_);
                }
                if (vertex_shader_) {
                    glDetachShader(program_, vertex_shader_);
                }
                glDeleteProgram(program_);
                program_ = 0;
            }
            if (fragment_shader_) {
                glDeleteShader(fragment_shader_);
                fragment_shader_ = 0;
            }
            if (vertex_shader_) {
                glDeleteShader(vertex_shader_);
                vertex_shader_ = 0;
            }
            if (index_buffer_) {
                glDeleteBuffers(1, &index_buffer_);
                index_buffer_ = 0;
            }
            if (vertex_buffer_) {
                glDeleteBuffers(1, &vertex_buffer_);
                vertex_buffer_ = 0;
            }
        }

        void draw_gl() throw() {
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view_mat = std::move(glm::lookAt(
                glm::vec3(0.0f, 1.0f, 3.0f),  // camera location
                glm::vec3(0.0f, 0.0f, 0.0f),  // looking at
                glm::vec3(0.0f, 1.0f, 0.0f)   // up direction
            ));
            glm::mat4 proj_mat = std::move(glm::perspective(
                glm::radians(52.0f),  // fov
                float(kWindowWidth)/float(kWindowHeight),   // aspect ratio
                0.1f,  // near clipping plane
                30.0f  // far  clipping plane
            ));
            glm::mat4 proj_view_mat = proj_mat * view_mat;

            glUseProgram(program_);
            glUniformMatrix4fv(proj_view_mat_loc_, 1, GL_FALSE, &proj_view_mat[0][0]);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(0);
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
