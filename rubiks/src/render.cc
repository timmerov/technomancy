/*
Copyright (C) 2012-2017 tim cotter. All rights reserved.
*/

/**
spinning rubik's cube.

the rubik's cube is a 3x3x3 stack of identical colored cubes.
we don't render the center cube.
cause it can never be seen.
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

#if !defined(XK_MISCELLANY)
#define XK_MISCELLANY 1
#endif
#include <X11/keysymdef.h>


namespace {
    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        uniform mat4 model_mat;
        uniform mat4 proj_view_mat;
        void main() {
            vec4 world_pos = model_mat * vec4(vertex_pos_in, 1.0f);
            gl_Position = proj_view_mat * world_pos;
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        out mediump vec4 color_out;
        uniform mediump vec3 color;
        void main() {
			color_out.rgb = color;
            color_out.a = 1.0f;
        }
    )shader_code";

	const int kNumCubes = 26;
	const int kNumStates = 24;

    const GLfloat kA = 0.50f;  /// sub-cube size
    const GLfloat kB = 0.45f;  /// sticker size
    const GLfloat kC = 0.48f;  /// bevel
    const GLfloat g_cube_vertexes[] {
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
	const GLfloat g_colors[] = {
		1.0f, 1.0f, 1.0f, /// white
		0.9f, 0.9f, 0.0f, /// yellow
		0.0f, 0.3f, 0.0f, /// green
		0.0f, 0.0f, 0.8f, /// blue
		0.7f, 0.0f, 0.0f, /// red
		1.0f, 0.5f, 0.0f, /// orange
	};
	const GLushort g_bevel_indexes[] = {
		8*0+0, 8*0+1, 8*0+2,
		8*0+0, 8*0+2, 8*0+4,
		8*0+0, 8*0+4, 8*0+6,
		8*0+1, 8*0+3, 8*0+2,
		8*0+1, 8*0+7, 8*0+3,
		8*0+3, 8*0+7, 8*0+5,
		8*0+4, 8*0+5, 8*0+6,
		8*0+5, 8*0+7, 8*0+6,

		8*1+0, 8*1+1, 8*1+2,
		8*1+0, 8*1+2, 8*1+4,
		8*1+0, 8*1+4, 8*1+6,
		8*1+1, 8*1+3, 8*1+2,
		8*1+1, 8*1+7, 8*1+3,
		8*1+3, 8*1+7, 8*1+5,
		8*1+4, 8*1+5, 8*1+6,
		8*1+5, 8*1+7, 8*1+6,

		8*2+0, 8*2+1, 8*2+2,
		8*2+0, 8*2+2, 8*2+4,
		8*2+0, 8*2+4, 8*2+6,
		8*2+1, 8*2+3, 8*2+2,
		8*2+1, 8*2+7, 8*2+3,
		8*2+3, 8*2+7, 8*2+5,
		8*2+4, 8*2+5, 8*2+6,
		8*2+5, 8*2+7, 8*2+6,

		8*3+0, 8*3+1, 8*3+2,
		8*3+0, 8*3+2, 8*3+4,
		8*3+0, 8*3+4, 8*3+6,
		8*3+1, 8*3+3, 8*3+2,
		8*3+1, 8*3+7, 8*3+3,
		8*3+3, 8*3+7, 8*3+5,
		8*3+4, 8*3+5, 8*3+6,
		8*3+5, 8*3+7, 8*3+6,

		8*4+0, 8*4+1, 8*4+2,
		8*4+0, 8*4+2, 8*4+4,
		8*4+0, 8*4+4, 8*4+6,
		8*4+1, 8*4+3, 8*4+2,
		8*4+1, 8*4+7, 8*4+3,
		8*4+3, 8*4+7, 8*4+5,
		8*4+4, 8*4+5, 8*4+6,
		8*4+5, 8*4+7, 8*4+6,

		8*5+0, 8*5+1, 8*5+2,
		8*5+0, 8*5+2, 8*5+4,
		8*5+0, 8*5+4, 8*5+6,
		8*5+1, 8*5+3, 8*5+2,
		8*5+1, 8*5+7, 8*5+3,
		8*5+3, 8*5+7, 8*5+5,
		8*5+4, 8*5+5, 8*5+6,
		8*5+5, 8*5+7, 8*5+6,
	};
	const GLushort g_face_indexes[] = {
		8*0+2, 8*0+3, 8*0+4,
		8*0+3, 8*0+5, 8*0+4,
		8*1+2, 8*1+3, 8*1+4,
		8*1+3, 8*1+5, 8*1+4,
		8*2+2, 8*2+3, 8*2+4,
		8*2+3, 8*2+5, 8*2+4,
		8*3+2, 8*3+3, 8*3+4,
		8*3+3, 8*3+5, 8*3+4,
		8*4+2, 8*4+3, 8*4+4,
		8*4+3, 8*4+5, 8*4+4,
		8*5+2, 8*5+3, 8*5+4,
		8*5+3, 8*5+5, 8*5+4,
	};
	const glm::vec3 g_xyz[26] = {
		{-1.0f, -1.0f, -1.0f},
		{-1.0f, -1.0f, +0.0f},
		{-1.0f, -1.0f, +1.0f},
		{-1.0f, +0.0f, -1.0f},
		{-1.0f, +0.0f, +0.0f},
		{-1.0f, +0.0f, +1.0f},
		{-1.0f, +1.0f, -1.0f},
		{-1.0f, +1.0f, +0.0f},
		{-1.0f, +1.0f, +1.0f},

		{+0.0f, -1.0f, -1.0f},
		{+0.0f, -1.0f, +0.0f},
		{+0.0f, -1.0f, +1.0f},
		{+0.0f, +0.0f, -1.0f},
		{+0.0f, +0.0f, +1.0f},
		{+0.0f, +1.0f, -1.0f},
		{+0.0f, +1.0f, +0.0f},
		{+0.0f, +1.0f, +1.0f},

		{+1.0f, -1.0f, -1.0f},
		{+1.0f, -1.0f, +0.0f},
		{+1.0f, -1.0f, +1.0f},
		{+1.0f, +0.0f, -1.0f},
		{+1.0f, +0.0f, +0.0f},
		{+1.0f, +0.0f, +1.0f},
		{+1.0f, +1.0f, -1.0f},
		{+1.0f, +1.0f, +0.0f},
		{+1.0f, +1.0f, +1.0f}
	};

	const glm::mat4 g_ident = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotx2 = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, -1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotxp = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, -1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotxm = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_roty2 = {
		-1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotyp = {
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotym = {
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		-1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotzp = {
		+0.0f, +1.0f, +0.0f, +0.0f,
		-1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotzm = {
		+0.0f, -1.0f, +0.0f, +0.0f,
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rot_table[kNumStates] = {
		g_ident, g_rotxp,         g_rotx2,         g_rotxm,
		g_rotyp, g_rotxp*g_rotyp, g_rotx2*g_rotyp, g_rotxm*g_rotyp,
		g_roty2, g_rotxp*g_roty2, g_rotx2*g_roty2, g_rotxm*g_roty2,
		g_rotym, g_rotxp*g_rotym, g_rotx2*g_rotym, g_rotxm*g_rotym,
		g_rotzp, g_rotxp*g_rotzp, g_rotx2*g_rotzp, g_rotxm*g_rotzp,
		g_rotzm, g_rotxp*g_rotzm, g_rotx2*g_rotzm, g_rotxm*g_rotzm
	};


    class RenderImpl : public Render {
    public:
        RenderImpl() = default;
        virtual ~RenderImpl() = default;

        int width_ = 0;
        int height_ = 0;
        GLuint vertex_buffer_ = 0;
        GLuint bevel_index_buffer_ = 0;
        GLuint face_index_buffer_[6] = {0};
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint model_mat_loc_ = 0;
        GLuint proj_view_mat_loc_ = 0;
        GLuint color_loc_ = 0;
        int num_bevel_indexes_ = 0;
        int num_face_indexes_ = 0;
        float angle_ = 0.0f;
        int frame_count_ = 0;
		int state_[kNumCubes] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0,    0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0
		};
		int update_counter_ = 0;

        virtual void init(
            int width,
            int height
        ) noexcept {
            width_ = width;
            height_ = height;

            float pi = (float) acos(-1.0f);
            angle_ = 2.0f*pi/7.0f;

            int num_vertex_floats = 3 * 8 * 6;
            num_bevel_indexes_ = 3 * 8 * 6;
            num_face_indexes_ = 3 * 2;

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);

            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_vertex_floats, g_cube_vertexes, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &bevel_index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bevel_index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_bevel_indexes_, g_bevel_indexes, GL_STATIC_DRAW);
            LOG("bevel_index=" << bevel_index_buffer_);

            glGenBuffers(6, &face_index_buffer_[0]);
            for (int i = 0; i < 6; ++i) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face_index_buffer_[i]);
				auto face_indexes = &g_face_indexes[num_face_indexes_*i];
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_face_indexes_, face_indexes, GL_STATIC_DRAW);
				LOG("face_index[" << i << "]=" << face_index_buffer_[i]);
			}

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
            color_loc_ = glGetUniformLocation(program_, "color");
            LOG("color_loc=" << color_loc_);
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
            if (bevel_index_buffer_) {
                glDeleteBuffers(1, &bevel_index_buffer_);
                bevel_index_buffer_ = 0;
            }
            for (int i = 0; i < 6; ++i) {
				if (face_index_buffer_[i]) {
					glDeleteBuffers(1, &face_index_buffer_[i]);
					face_index_buffer_[i] = 0;
				}
			}
            if (vertex_buffer_) {
                glDeleteBuffers(1, &vertex_buffer_);
                vertex_buffer_ = 0;
            }
        }

        virtual void draw() noexcept {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            /*float pi = (float) acos(-1.0f);
            angle_ += 2.0f*pi/60.0f/12.0f;
            if (angle_ >= 2.0f*pi) {
                angle_ -= 2.0f*pi;
            }*/

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

			drawAllCubes(rot_mat);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(0);
            glUseProgram(0);

			//captureFrame();
        }

        void drawAllCubes(
			glm::mat4& rot_mat
		) noexcept {
			for (int i = 0; i < kNumCubes; ++i) {
				int state = state_[i];
				glm::mat4 temp_mat = std::move(glm::translate(
					rot_mat,
					g_xyz[i]
				));
				glm::mat4 model_mat = temp_mat * g_rot_table[state];
				glUniformMatrix4fv(model_mat_loc_, 1, GL_FALSE, &model_mat[0][0]);
				draw1Cube();
			}
        }

        void draw1Cube() noexcept {
			GLfloat dark[] = {0.1f, 0.1f, 0.1f};
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bevel_index_buffer_);
			glUniform3fv(color_loc_, 1, &dark[0]);
			glDrawElements(GL_TRIANGLES, num_bevel_indexes_, GL_UNSIGNED_SHORT, nullptr);

			for (int i = 0; i < 6; ++i) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face_index_buffer_[i]);
				glUniform3fv(color_loc_, 1, &g_colors[3*i]);
				glDrawElements(GL_TRIANGLES, num_face_indexes_, GL_UNSIGNED_SHORT, nullptr);
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
            ss << "output/rubiks" << std::setfill('0') << std::setw(5) << frame_count_
                << std::setfill(' ') << std::setw(0) << ".png";
            LOG("ss=" << ss.str().c_str());
            png.write(ss.str().c_str());

            ++frame_count_;
        }

		virtual void keyPressed(
			int symbol
		) noexcept {
			switch (symbol) {
				case XK_Down: {
					for (int i = 0; i < kNumCubes; ++i) {
						int state = state_[i];
						int a = state % 4;
						int b = state / 4;
						a = (a + 1) % 4;
						state = a + 4*b;
						state_[i] = state;
					}
					break;
				}
			}
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
