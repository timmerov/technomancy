/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

/**
spinning jupiter example.

much help from here.
https://github.com/opengl-tutorials/ogl

*** caution ***
we're going to use tri-linear interpolation.
which means we're going to generate mip-maps.
which means the top strip is going to bleed into
the bottom strip.
resulting in a visible seam.
there's not much we can do about it.
other than create separate textures for the top
and bottom strips.
and render them separately.
**/

#include "render.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/opengl.h>
#include <common/png.h>
#include <common/sphere.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

// sigh...
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <sstream>
#include <iomanip>


namespace {
    const int kNumSegments = 12;
    const char kTextureFilename[] = "cube-sharp50.png";

    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        layout (location = 1) in vec3 color_in;
        out mediump vec3 color;
        uniform mat4 model_mat;
        uniform mat4 proj_view_mat;
        void main() {
            vec4 world_pos = model_mat * vec4(vertex_pos_in, 1.0f);
            gl_Position = proj_view_mat * world_pos;
            color = color_in;
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        in mediump vec3 color;
        out mediump vec4 color_out;
        void main() {
            color_out.rgb = color.rgb;
            color_out.a = 1.0f;
        }
    )shader_code";

    class RenderImpl : public Render {
    public:
        RenderImpl() = default;
        virtual ~RenderImpl() = default;

        int width_ = 0;
        int height_ = 0;
        GLuint vertex_buffer_ = 0;
        GLuint color_buffer_ = 0;
        GLuint index_buffer_ = 0;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint model_mat_loc_ = 0;
        GLuint proj_view_mat_loc_ = 0;
        int num_indexes_ = 0;
        float angle_ = 0.0f;
        int frame_count_ = 0;

        virtual void init(
            int width,
            int height
        ) noexcept {
            width_ = width;
            height_ = height;

            int num_segments = 1;
            sphere::Sphere sphere;
            {
                sphere::Gen gen;
                gen.generate(num_segments, &sphere);
            }
            LOG("num_segments=" << num_segments << " num_vertexes=" << sphere.num_vertexes_ << " num_faces=" << sphere.num_faces_);
            if (sphere.num_vertexes_ > 65535) {
                LOG("*** ERROR *** required index range exceeds unsigned short.");
            }

            int num_vertex_floats = 3 * sphere.num_vertexes_;
            auto vertex_array = new(std::nothrow) GLfloat[num_vertex_floats];
            for (int i = 0; i < sphere.num_vertexes_; ++i) {
                vertex_array[3*i+0] = (GLfloat) sphere.vertex_[i].x_;
                vertex_array[3*i+1] = (GLfloat) sphere.vertex_[i].y_;
                vertex_array[3*i+2] = (GLfloat) sphere.vertex_[i].z_;
                //LOG("vertex[" << i << "]={" << vertex_array_[3*i+0] << ", " << vertex_array_[3*i+1] << ", " << vertex_array_[3*i+2] << "}");
            }
            int num_color_floats = 3 * sphere.num_vertexes_;
            static GLfloat g_colors[] = {
				1.0f, 1.0f, 1.0f, /// white
				0.0f, 1.0f, 0.0f, /// green
				1.0f, 1.0f, 0.0f, /// yellow
				1.0f, 0.0f, 0.0f, /// red
				0.0f, 0.0f, 1.0f, /// blue
				1.0f, 0.5f, 0.0f  /// orange
            };
            auto color_array = new(std::nothrow) GLfloat[num_color_floats];
            for (int i = 0; i < 6; ++i) {
				for (int k = 0; k < 4; ++k) {
					color_array[4*3*i+3*k+0] = g_colors[3*i+0];
					color_array[4*3*i+3*k+1] = g_colors[3*i+1];
					color_array[4*3*i+3*k+2] = g_colors[3*i+2];
				}
			}
            num_indexes_ = 3 * sphere.num_faces_;
            auto index_array = new(std::nothrow) GLushort[num_indexes_];
            for (int i = 0; i < sphere.num_faces_; ++i) {
                index_array[3*i+0] = (GLushort) sphere.face_[i].a_;
                index_array[3*i+1] = (GLushort) sphere.face_[i].b_;
                index_array[3*i+2] = (GLushort) sphere.face_[i].c_;
                //LOG("face[" << i << "]={" << index_array_[3*i+0] << ", " << index_array_[3*i+1] << ", " << index_array_[3*i+2] << "}");
            }

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);

            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_vertex_floats, vertex_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &color_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, color_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_color_floats, color_array, GL_STATIC_DRAW);
            LOG("color=" << color_buffer_);

            glGenBuffers(1, &index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_indexes_, index_array, GL_STATIC_DRAW);
            LOG("index=" << index_buffer_);

            vertex_shader_ = agm::gl::compileShader(GL_VERTEX_SHADER, g_vertex_source);
            LOG("vertex_shader=" << vertex_shader_);
            fragment_shader_ = agm::gl::compileShader(GL_FRAGMENT_SHADER, g_fragment_source);
            LOG("fragment_shader=" << fragment_shader_);

            program_ = agm::gl::linkProgram(vertex_shader_, fragment_shader_);
            LOG("program=" << program_);

            glUseProgram(program_);
            model_mat_loc_ = glGetUniformLocation(program_, "model_mat");
            LOG("model_mat_loc=" << model_mat_loc_);
            proj_view_mat_loc_ = glGetUniformLocation(program_, "proj_view_mat");
            LOG("proj_view_mat_loc=" << proj_view_mat_loc_);

            delete[] index_array;
            delete[] color_array;
            delete[] vertex_array;
        }

        virtual void exit() noexcept {
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
            if (color_buffer_) {
                glDeleteBuffers(1, &color_buffer_);
                color_buffer_ = 0;
            }
            if (vertex_buffer_) {
                glDeleteBuffers(1, &vertex_buffer_);
                vertex_buffer_ = 0;
            }
        }

        virtual void draw() noexcept {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float pi = (float) acos(-1.0f);
            angle_ += 2.0f*pi/60.0f/12.0f;
            if (angle_ >= 2.0f*pi) {
                angle_ -= 2.0f*pi;
            }
            glm::mat4 model_mat = std::move(glm::rotate(
                glm::mat4(),
                angle_,
                glm::vec3(0.0f, 1.0f, 0.0f)  // around y axis
            ));

            glm::mat4 view_mat = std::move(glm::lookAt(
                glm::vec3(0.0f, 1.0f, 2.5f),  // camera location
                glm::vec3(0.0f, 0.0f, 0.0f),  // looking at
                glm::vec3(0.0f, 1.0f, 0.0f)   // up direction
            ));
            glm::mat4 proj_mat = std::move(glm::perspective(
                glm::radians(52.0f),  // fov
                float(width_)/float(height_),   // aspect ratio
                0.1f,  // near clipping plane
                30.0f  // far  clipping plane
            ));
            glm::mat4 proj_view_mat = proj_mat * view_mat;

            glViewport(0, 0, width_, height_);

            glUseProgram(program_);
            glUniformMatrix4fv(model_mat_loc_, 1, GL_FALSE, &model_mat[0][0]);
            glUniformMatrix4fv(proj_view_mat_loc_, 1, GL_FALSE, &proj_view_mat[0][0]);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, color_buffer_);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glDrawElements(GL_TRIANGLES, num_indexes_, GL_UNSIGNED_SHORT, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);
            glUseProgram(0);

            //captureFrame();
        }

        virtual void resize(
            int width,
            int height
        ) noexcept {
            width_ = width;
            height_ = height;
        }

        void captureFrame() noexcept {
            if (frame_count_ >= 48*60) {
                return;
            }

            Png png_flipped;
            png_flipped.init(width_, height_);
            glReadPixels(0, 0, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, png_flipped.data_);
            //png_flipped.write("flipped.png");

            Png png;
            png.init(width_, height_);
            auto src = png_flipped.data_ + (height_ - 1)*png_flipped.stride_;
            auto dst = png.data_;
            for (int i = 0; i < height_; ++i) {
                memcpy(dst, src, png.stride_);
                src -= png_flipped.stride_;
                dst += png.stride_;
            }

            std::stringstream ss;
            ss << "output/world" << std::setfill('0') << std::setw(5) << frame_count_
                << std::setfill(' ') << std::setw(0) << ".png";
            LOG("ss=" << ss.str().c_str());
            png.write(ss.str().c_str());

            ++frame_count_;
        }
    };
}

Render::Render() noexcept {
}

Render::~Render() noexcept {
}

Render *Render::create() noexcept {
    auto impl = new(std::nothrow) RenderImpl;
    return impl;
}
