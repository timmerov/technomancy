/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning world example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/sphere.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render.h"


namespace {
    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        layout (location = 1) in vec2 texture_pos_in;
        out mediump vec2 texture_pos;
        uniform mat4 proj_view_mat;
        void main() {
            gl_Position = proj_view_mat * vec4(vertex_pos_in, 1.0f);
            texture_pos = texture_pos_in;
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        in mediump vec2 texture_pos;
        out mediump vec4 frag_color;
        void main() {
            frag_color = vec4(texture_pos.x, texture_pos.y, 0.0f, 1.0f);
        }
    )shader_code";

    class RenderImpl : public Render {
    public:
        RenderImpl() = default;
        virtual ~RenderImpl() = default;

        int width_ = 0;
        int height_ = 0;
        int num_triangles_ = 0;
        GLuint vertex_id_ = 0;
        GLuint vertex_buffer_ = 0;
        GLuint texture_buffer_ = 0;
        GLuint index_buffer_ = 0;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint proj_view_mat_loc_ = 0;
        int num_indexes_ = 0;

        virtual void init(
            int width,
            int height
        ) throw() {
            width_ = width;
            height_ = height;

            sphere::Sphere sphere;
            {
                sphere::Gen gen;
                gen.generate(3, &sphere);
            }
            LOG("num_vertexes=" << sphere.num_vertexes_ << " num_faces=" << sphere.num_faces_);

            int num_vertex_floats = 3 * sphere.num_vertexes_;
            auto vertex_array = new(std::nothrow) GLfloat[num_vertex_floats];
            for (int i = 0; i < sphere.num_vertexes_; ++i) {
                vertex_array[3*i+0] = (GLfloat) sphere.vertex_[i].x_;
                vertex_array[3*i+1] = (GLfloat) sphere.vertex_[i].y_;
                vertex_array[3*i+2] = (GLfloat) sphere.vertex_[i].z_;
                //LOG("vertex[" << i << "]={" << vertex_array_[3*i+0] << ", " << vertex_array_[3*i+1] << ", " << vertex_array_[3*i+2] << "}");
            }
            int num_texture_floats = 2 * sphere.num_vertexes_;
            auto texture_array = new(std::nothrow) GLfloat[num_texture_floats];
            for (int i = 0; i < sphere.num_vertexes_; ++i) {
                texture_array[2*i+0] = (GLfloat) sphere.texture_[i].x_;
                texture_array[2*i+1] = (GLfloat) sphere.texture_[i].y_;
            }
            num_indexes_ = 3 * sphere.num_faces_;
            auto index_array = new(std::nothrow) GLushort[num_indexes_];
            for (int i = 0; i < sphere.num_faces_; ++i) {
                index_array[3*i+0] = (GLushort) sphere.face_[i].a_;
                index_array[3*i+1] = (GLushort) sphere.face_[i].b_;
                index_array[3*i+2] = (GLushort) sphere.face_[i].c_;
                //LOG("face[" << i << "]={" << index_array_[3*i+0] << ", " << index_array_[3*i+1] << ", " << index_array_[3*i+2] << "}");
            }
            num_triangles_ = sphere.num_faces_;

            glViewport(0, 0, width, height);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDisable(GL_CULL_FACE);

            glGenVertexArrays(1, &vertex_id_);
            glBindVertexArray(vertex_id_);

            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_vertex_floats, vertex_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &texture_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_texture_floats, texture_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_indexes_, index_array, GL_STATIC_DRAW);
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

            delete[] index_array;
            delete[] texture_array;
            delete[] vertex_array;
        }

        virtual void exit() throw() {
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
            if (texture_buffer_) {
                glDeleteBuffers(1, &texture_buffer_);
                texture_buffer_ = 0;
            }
            if (vertex_buffer_) {
                glDeleteBuffers(1, &vertex_buffer_);
                vertex_buffer_ = 0;
            }
            if (vertex_id_) {
                glDeleteVertexArrays(1, &vertex_id_);
                vertex_id_ = 0;
            }
        }

        virtual void draw() throw() {
            glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view_mat = std::move(glm::lookAt(
                glm::vec3(0.0f, 1.0f, 3.0f),  // camera location
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

            glUseProgram(program_);
            glUniformMatrix4fv(proj_view_mat_loc_, 1, GL_FALSE, &proj_view_mat[0][0]);

            glBindVertexArray(vertex_id_);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, texture_buffer_);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

            glDrawElements(GL_TRIANGLES, num_indexes_, GL_UNSIGNED_SHORT, nullptr);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(1);
            glDisableVertexAttribArray(0);
            glBindVertexArray(0);
            glUseProgram(0);
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
    };
}

Render::Render() throw() {
}

Render::~Render() throw() {
}

Render *Render::create() throw() {
    auto impl = new(std::nothrow) RenderImpl;
    return impl;
}
