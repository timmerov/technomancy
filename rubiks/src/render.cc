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

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>

// sigh...
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <sstream>
#include <iomanip>


namespace {
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
			color_out.rgb = color;
            color_out.a = 1.0f;
        }
    )shader_code";

    const GLfloat kA = 0.50f;
    const GLfloat kB = 0.45f;
    const GLfloat kC = 0.48f;
    static GLfloat g_cube_vertexes[] {
		+kC, +kC, +kC, /// 0+8*0
		-kC, +kC, +kC, /// 1+8*0
		+kB, +kB, +kA, /// 2+8*0
		-kB, +kB, +kA, /// 3+8*0
		+kB, -kB, +kA, /// 4+8*0
		-kB, -kB, +kA, /// 5+8*0
		+kC, -kC, +kC, /// 6+8*0
		-kC, -kC, +kC, /// 7+8*0

		-kC, +kC, -kC, /// 0+8*1
		+kC, +kC, -kC, /// 1+8*1
		-kB, +kB, -kA, /// 2+8*1
		+kB, +kB, -kA, /// 3+8*1
		-kB, -kB, -kA, /// 4+8*1
		+kB, -kB, -kA, /// 5+8*1
		-kC, -kC, -kC, /// 6+8*1
		+kC, -kC, -kC, /// 7+8*1

		+kC, +kC, +kC, /// 0+8*2
		+kC, -kC, +kC, /// 1+8*2
		+kA, +kB, +kB, /// 2+8*2
		+kA, -kB, +kB, /// 3+8*2
		+kA, +kB, -kB, /// 4+8*2
		+kA, -kB, -kB, /// 5+8*2
		+kC, +kC, -kC, /// 6+8*2
		+kC, -kC, -kC, /// 7+8*2

		-kC, +kC, -kC, /// 0+8*3
		-kC, -kC, -kC, /// 1+8*3
		-kA, +kB, -kB, /// 2+8*3
		-kA, -kB, -kB, /// 3+8*3
		-kA, +kB, +kB, /// 4+8*3
		-kA, -kB, +kB, /// 5+8*3
		-kC, +kC, +kC, /// 6+8*3
		-kC, -kC, +kC, /// 7+8*3

		-kC, +kC, +kC, /// 0+8*4
		+kC, +kC, +kC, /// 1+8*4
		-kB, +kA, +kB, /// 2+8*4
		+kB, +kA, +kB, /// 3+8*4
		-kB, +kA, -kB, /// 4+8*4
		+kB, +kA, -kB, /// 5+8*4
		-kC, +kC, -kC, /// 6+8*4
		+kC, +kC, -kC, /// 7+8*4

		-kC, -kC, -kC, /// 0+8*5
		+kC, -kC, -kC, /// 1+8*5
		-kB, -kA, -kB, /// 2+8*5
		+kB, -kA, -kB, /// 3+8*5
		-kB, -kA, +kB, /// 4+8*5
		+kB, -kA, +kB, /// 5+8*5
		-kC, -kC, +kC, /// 6+8*5
		+kC, -kC, +kC, /// 7+8*5
	};
	static GLfloat g_colors[] = {
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
		1.0f, 1.0f, 1.0f, /// white
		1.0f, 1.0f, 1.0f, /// white
		1.0f, 1.0f, 1.0f, /// white
		1.0f, 1.0f, 1.0f, /// white
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark

		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
		1.0f, 1.0f, 0.0f, /// yellow
		1.0f, 1.0f, 0.0f, /// yellow
		1.0f, 1.0f, 0.0f, /// yellow
		1.0f, 1.0f, 0.0f, /// yellow
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark

		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
		1.0f, 0.0f, 0.0f, /// red
		1.0f, 0.0f, 0.0f, /// red
		1.0f, 0.0f, 0.0f, /// red
		1.0f, 0.0f, 0.0f, /// red
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark

		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
		1.0f, 0.5f, 0.0f, /// orange
		1.0f, 0.5f, 0.0f, /// orange
		1.0f, 0.5f, 0.0f, /// orange
		1.0f, 0.5f, 0.0f, /// orange
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark

		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
		0.0f, 0.0f, 1.0f, /// blue
		0.0f, 0.0f, 1.0f, /// blue
		0.0f, 0.0f, 1.0f, /// blue
		0.0f, 0.0f, 1.0f, /// blue
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark

		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
		0.0f, 1.0f, 0.0f, /// green
		0.0f, 1.0f, 0.0f, /// green
		0.0f, 1.0f, 0.0f, /// green
		0.0f, 1.0f, 0.0f, /// green
		0.1f, 0.1f, 0.1f, /// dark
		0.1f, 0.1f, 0.1f, /// dark
	};
	static GLushort g_cube_indexes[] = {
		8*0+0, 8*0+1, 8*0+2,
		8*0+0, 8*0+2, 8*0+4,
		8*0+0, 8*0+4, 8*0+6,
		8*0+1, 8*0+3, 8*0+2,
		8*0+1, 8*0+7, 8*0+3,
		8*0+2, 8*0+3, 8*0+4,
		8*0+3, 8*0+5, 8*0+4,
		8*0+3, 8*0+7, 8*0+5,
		8*0+4, 8*0+5, 8*0+6,
		8*0+5, 8*0+7, 8*0+6,

		8*1+0, 8*1+1, 8*1+2,
		8*1+0, 8*1+2, 8*1+4,
		8*1+0, 8*1+4, 8*1+6,
		8*1+1, 8*1+3, 8*1+2,
		8*1+1, 8*1+7, 8*1+3,
		8*1+2, 8*1+3, 8*1+4,
		8*1+3, 8*1+5, 8*1+4,
		8*1+3, 8*1+7, 8*1+5,
		8*1+4, 8*1+5, 8*1+6,
		8*1+5, 8*1+7, 8*1+6,

		8*2+0, 8*2+1, 8*2+2,
		8*2+0, 8*2+2, 8*2+4,
		8*2+0, 8*2+4, 8*2+6,
		8*2+1, 8*2+3, 8*2+2,
		8*2+1, 8*2+7, 8*2+3,
		8*2+2, 8*2+3, 8*2+4,
		8*2+3, 8*2+5, 8*2+4,
		8*2+3, 8*2+7, 8*2+5,
		8*2+4, 8*2+5, 8*2+6,
		8*2+5, 8*2+7, 8*2+6,

		8*3+0, 8*3+1, 8*3+2,
		8*3+0, 8*3+2, 8*3+4,
		8*3+0, 8*3+4, 8*3+6,
		8*3+1, 8*3+3, 8*3+2,
		8*3+1, 8*3+7, 8*3+3,
		8*3+2, 8*3+3, 8*3+4,
		8*3+3, 8*3+5, 8*3+4,
		8*3+3, 8*3+7, 8*3+5,
		8*3+4, 8*3+5, 8*3+6,
		8*3+5, 8*3+7, 8*3+6,

		8*4+0, 8*4+1, 8*4+2,
		8*4+0, 8*4+2, 8*4+4,
		8*4+0, 8*4+4, 8*4+6,
		8*4+1, 8*4+3, 8*4+2,
		8*4+1, 8*4+7, 8*4+3,
		8*4+2, 8*4+3, 8*4+4,
		8*4+3, 8*4+5, 8*4+4,
		8*4+3, 8*4+7, 8*4+5,
		8*4+4, 8*4+5, 8*4+6,
		8*4+5, 8*4+7, 8*4+6,

		8*5+0, 8*5+1, 8*5+2,
		8*5+0, 8*5+2, 8*5+4,
		8*5+0, 8*5+4, 8*5+6,
		8*5+1, 8*5+3, 8*5+2,
		8*5+1, 8*5+7, 8*5+3,
		8*5+2, 8*5+3, 8*5+4,
		8*5+3, 8*5+5, 8*5+4,
		8*5+3, 8*5+7, 8*5+5,
		8*5+4, 8*5+5, 8*5+6,
		8*5+5, 8*5+7, 8*5+6,
	};

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

            int num_vertex_floats = 3 * 8 * 6;
            int num_color_floats = 3 * 8 * 6;
            num_indexes_ = 3 * 10 * 6;

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);

            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_vertex_floats, g_cube_vertexes, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &color_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, color_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_color_floats, g_colors, GL_STATIC_DRAW);
            LOG("color=" << color_buffer_);

            glGenBuffers(1, &index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_indexes_, g_cube_indexes, GL_STATIC_DRAW);
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

			glm::mat4 rot_mat = std::move(glm::rotate(
				glm::mat4(),
				angle_,
				glm::vec3(0.0f, 1.0f, 0.0f)  // around y axis
				//glm::vec3(0.0f, 0.0f, 1.0f)  // around z axis
				//glm::vec3(1.0f, 0.0f, 0.0f)  // around x axis
			));

            glm::mat4 view_mat = std::move(glm::lookAt(
                glm::vec3(0.0f, 8.0f, 12.0f),  // camera location
                glm::vec3(0.0f, 0.0f, 0.0f),  // looking at
                glm::vec3(0.0f, 1.0f, 0.0f)   // up direction
            ));
            glm::mat4 proj_mat = std::move(glm::perspective(
                glm::radians(26.0f),  // fov
                float(width_)/float(height_),   // aspect ratio
                0.1f,  // near clipping plane
                30.0f  // far  clipping plane
            ));
            glm::mat4 proj_view_mat = proj_mat * view_mat;

            glViewport(0, 0, width_, height_);

            glUseProgram(program_);
            glUniformMatrix4fv(proj_view_mat_loc_, 1, GL_FALSE, &proj_view_mat[0][0]);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, color_buffer_);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

			drawAllCubes(rot_mat);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);
            glUseProgram(0);

            //captureFrame();
        }

        void drawAllCubes(
			glm::mat4& rot_mat
		) noexcept {
			for (int x = -1; x < 2; ++x) {
				for (int y = -1; y < 2; ++y) {
					for (int z = -1; z < 2; ++z) {
						glm::mat4 model_mat = std::move(glm::translate(
							rot_mat,
							glm::vec3(float(x), float(y), float(z))
						));

						glUniformMatrix4fv(model_mat_loc_, 1, GL_FALSE, &model_mat[0][0]);
						glDrawElements(GL_TRIANGLES, num_indexes_, GL_UNSIGNED_SHORT, nullptr);
					}
				}
			}
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
