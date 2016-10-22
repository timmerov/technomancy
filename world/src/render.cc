/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

/**
spinning world example.
**/

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/png.h>
#include <common/sphere.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render.h"


namespace {
    const int kNumSegments = 12;
    const char kTextureFilename[] = "cube-sharp50.png";

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
        out mediump vec4 color_out;
        uniform sampler2D texture_sampler;
        void main() {
            color_out = texture(texture_sampler, texture_pos);
        }
    )shader_code";

    class RenderImpl : public Render {
    public:
        RenderImpl() = default;
        virtual ~RenderImpl() = default;

        int width_ = 0;
        int height_ = 0;
        int num_triangles_ = 0;
        GLuint vertex_array_ = 0;
        GLuint vertex_buffer_ = 0;
        GLuint coords_buffer_ = 0;
        GLuint index_buffer_ = 0;
        GLuint texture_ = 0;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint proj_view_mat_loc_ = 0;
        GLuint texture_loc_ = 0;
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
                gen.generate(kNumSegments, &sphere);
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
            int num_coords_floats = 2 * sphere.num_vertexes_;
            auto coords_array = new(std::nothrow) GLfloat[num_coords_floats];
            for (int i = 0; i < sphere.num_vertexes_; ++i) {
                coords_array[2*i+0] = (GLfloat) sphere.texture_[i].x_;
                coords_array[2*i+1] = (GLfloat) sphere.texture_[i].y_;
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

            glGenVertexArrays(1, &vertex_array_);
            glBindVertexArray(vertex_array_);

            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_vertex_floats, vertex_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &coords_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, coords_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_coords_floats, coords_array, GL_STATIC_DRAW);
            LOG("vertex=" << vertex_buffer_);

            glGenBuffers(1, &index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_indexes_, index_array, GL_STATIC_DRAW);
            LOG("index=" << index_buffer_);

            vertex_shader_ = compile_shader(GL_VERTEX_SHADER, g_vertex_source);
            LOG("vertex_shader=" << vertex_shader_);
            fragment_shader_ = compile_shader(GL_FRAGMENT_SHADER, g_fragment_source);
            LOG("fragment_shader=" << fragment_shader_);

            Png png;
            png.read(kTextureFilename);
            glGenTextures(1, &texture_);
            glBindTexture(GL_TEXTURE_2D, texture_);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, png.wd_, png.ht_, 0, GL_RGB, GL_UNSIGNED_BYTE, png.data_);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glGenerateMipmap(GL_TEXTURE_2D);

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
            texture_loc_ = glGetUniformLocation(program_, "texture_sampler");
            LOG("texture_loc=" << texture_loc_);

            delete[] index_array;
            delete[] coords_array;
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
            if (texture_) {
                glDeleteTextures(1, &texture_);
                texture_ = 0;
            }
            if (index_buffer_) {
                glDeleteBuffers(1, &index_buffer_);
                index_buffer_ = 0;
            }
            if (coords_buffer_) {
                glDeleteBuffers(1, &coords_buffer_);
                coords_buffer_ = 0;
            }
            if (vertex_buffer_) {
                glDeleteBuffers(1, &vertex_buffer_);
                vertex_buffer_ = 0;
            }
            if (vertex_array_) {
                glDeleteVertexArrays(1, &vertex_array_);
                vertex_array_ = 0;
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

            glBindVertexArray(vertex_array_);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, coords_buffer_);
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
