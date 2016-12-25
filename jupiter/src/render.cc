/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
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

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <common/png.h>
#include <common/sphere.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render.h"

#include <sstream>
#include <iomanip>


namespace {
    const int kNumSegments = 12;
    const char kTextureFilename[] = "jupiter-cube-sharp50.png";

    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        layout (location = 1) in vec2 texture_pos_in;
        out mediump vec2 texture_pos;
        out mediump float bright;
        uniform mat4 model_mat;
        uniform mat4 proj_view_mat;
        void main() {
            vec3 sun_dir = normalize(vec3(3.0f, 1.0f, 3.0f));
            vec4 world_pos = model_mat * vec4(vertex_pos_in, 1.0f);
            gl_Position = proj_view_mat * world_pos;
            texture_pos = texture_pos_in;
            bright = dot(world_pos.xyz, sun_dir);
            bright = clamp(bright, 0.0f, 1.0f);
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        in mediump vec2 texture_pos;
        in mediump float bright;
        out mediump vec4 color_out;
        uniform sampler2D texture_sampler;
        void main() {
            color_out = bright * texture(texture_sampler, texture_pos);
        }
    )shader_code";

    class SphereTexture {
    public:
        SphereTexture() = default;
        ~SphereTexture() = default;

        GLuint front_ = 0;
        GLuint back_ = 0;
    };

    class RenderImpl : public Render {
    public:
        RenderImpl() = default;
        virtual ~RenderImpl() = default;

        int width_ = 0;
        int height_ = 0;
        GLuint vertex_buffer_ = 0;
        GLuint coords_buffer_ = 0;
        GLuint index_buffer_ = 0;
        SphereTexture tex_;
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint model_mat_loc_ = 0;
        GLuint proj_view_mat_loc_ = 0;
        GLuint texture_loc_ = 0;
        int num_indexes_ = 0;
        float angle_ = 0.0f;
        glm::mat4 rotxz_;
        int frame_count_ = 0;

        virtual void init(
            int width,
            int height
        ) throw() {
            width_ = width;
            height_ = height;

            int num_segments = calcSegments(width, height);
            sphere::Sphere sphere;
            {
                sphere::Gen gen;
                gen.generate(num_segments, &sphere);
            }
            LOG("num_segments=" << num_segments << " num_vertexes=" << sphere.num_vertexes_ << " num_faces=" << sphere.num_faces_);
            if (sphere.num_vertexes_ > 65535) {
                LOG("*** ERROR *** required index range exceeds unsigned short.");
            }

            /*
            we only need half of the vertexes and coords.
            we're going to draw the sphere in two strips.
            the first half of the vertexes will be rotated to draw the second half.
            */
            int num_vertex_floats = 3 * sphere.num_vertexes_ / 2;
            auto vertex_array = new(std::nothrow) GLfloat[num_vertex_floats];
            for (int i = 0; i < sphere.num_vertexes_ / 2; ++i) {
                vertex_array[3*i+0] = (GLfloat) sphere.vertex_[i].x_;
                vertex_array[3*i+1] = (GLfloat) sphere.vertex_[i].y_;
                vertex_array[3*i+2] = (GLfloat) sphere.vertex_[i].z_;
                //LOG("vertex[" << i << "]={" << vertex_array_[3*i+0] << ", " << vertex_array_[3*i+1] << ", " << vertex_array_[3*i+2] << "}");
            }
            int num_coords_floats = 2 * sphere.num_vertexes_ / 2;
            auto coords_array = new(std::nothrow) GLfloat[num_coords_floats];
            for (int i = 0; i < sphere.num_vertexes_ / 2; ++i) {
                coords_array[2*i+0] = (GLfloat) sphere.texture_[i].x_;
                coords_array[2*i+1] = 2.0f * (GLfloat) sphere.texture_[i].y_;
            }
            num_indexes_ = 3 * sphere.num_faces_ / 2;
            auto index_array = new(std::nothrow) GLushort[num_indexes_];
            for (int i = 0; i < sphere.num_faces_ / 2; ++i) {
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

            glGenBuffers(1, &coords_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, coords_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_coords_floats, coords_array, GL_STATIC_DRAW);
            LOG("coords=" << coords_buffer_);

            glGenBuffers(1, &index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_indexes_, index_array, GL_STATIC_DRAW);
            LOG("index=" << index_buffer_);

            vertex_shader_ = compileShader(GL_VERTEX_SHADER, g_vertex_source);
            LOG("vertex_shader=" << vertex_shader_);
            fragment_shader_ = compileShader(GL_FRAGMENT_SHADER, g_fragment_source);
            LOG("fragment_shader=" << fragment_shader_);

            program_ = linkProgram(vertex_shader_, fragment_shader_);
            LOG("program=" << program_);

            loadPng(kTextureFilename, &tex_);

            glUseProgram(program_);
            model_mat_loc_ = glGetUniformLocation(program_, "model_mat");
            LOG("model_mat_loc=" << model_mat_loc_);
            proj_view_mat_loc_ = glGetUniformLocation(program_, "proj_view_mat");
            LOG("proj_view_mat_loc=" << proj_view_mat_loc_);
            texture_loc_ = glGetUniformLocation(program_, "texture_sampler");
            LOG("texture_loc=" << texture_loc_);

            glUseProgram(program_);
            glUniform1i(texture_loc_, 0);
            glUseProgram(0);

            rotxz_[0][0] = -1.0f;
            rotxz_[0][1] = +0.0f;
            rotxz_[0][2] = +0.0f;
            rotxz_[1][0] = +0.0f;
            rotxz_[1][1] = +0.0f;
            rotxz_[1][2] = +1.0f;
            rotxz_[2][0] = +0.0f;
            rotxz_[2][1] = +1.0f;
            rotxz_[2][2] = +0.0f;

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
            if (tex_.back_) {
                glDeleteTextures(1, &tex_.back_);
                tex_.back_ = 0;
            }
            if (tex_.front_) {
                glDeleteTextures(1, &tex_.front_);
                tex_.front_ = 0;
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
        }

        virtual void draw() throw() {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            float pi = (float) acos(-1.0f);
            angle_ += 2.0f*pi/60.0f/48.0f;  // 24 hours in 48 seconds.
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
            glBindBuffer(GL_ARRAY_BUFFER, coords_buffer_);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex_.front_);
            glDrawElements(GL_TRIANGLES, num_indexes_, GL_UNSIGNED_SHORT, nullptr);

            model_mat = model_mat * rotxz_;
            glUniformMatrix4fv(model_mat_loc_, 1, GL_FALSE, &model_mat[0][0]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex_.back_);
            glDrawElements(GL_TRIANGLES, num_indexes_, GL_UNSIGNED_SHORT, nullptr);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE0);
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
        ) throw() {
            int cur_segments = calcSegments(width_, height_);
            int new_segments = calcSegments(width, height);

            width_ = width;
            height_ = height;

            if (cur_segments != new_segments) {

                exit();
                init(width, height);
            }
        }

        GLuint compileShader(
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

        GLuint linkProgram(
            GLuint vertex_shader,
            GLuint fragment_shader
        ) throw() {
            GLuint program = glCreateProgram();
            glAttachShader(program, vertex_shader);
            glAttachShader(program, fragment_shader);
            glLinkProgram(program);

            GLint result = GL_FALSE;
            int len;
            glGetProgramiv(program, GL_LINK_STATUS, &result);
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            if (len > 0) {
                auto info = new(std::nothrow) char[len+1];
                glGetProgramInfoLog(program, len, nullptr, info);
                LOG("error log: " << info);
                delete[] info;
            }

            return program;
        }

        void loadPng(
            const char *filename,
            SphereTexture *texture
        ) throw() {
            Png png;
            png.read(filename);
            auto ht2 = png.ht_ / 2;
            auto data = png.data_;
            texture->front_ = loadTexture(png.wd_, ht2, data);
            LOG("front=" << texture->front_);
            data += png.stride_ * ht2;
            texture->back_ = loadTexture(png.wd_, ht2, data);
            LOG("back=" << texture->back_);
        }

        GLuint loadTexture(
            int width,
            int height,
            GLubyte *data
        ) throw() {
            GLuint texture = 0;
            glGenTextures(1, &texture);

            glBindTexture(GL_TEXTURE_2D, texture);
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glGenerateMipmap(GL_TEXTURE_2D);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            return texture;
        }

        int calcSegments(
            int width,
            int height
        ) throw() {
            // oddly, does not depend on width.
            (void) width;
            auto segs = kNumSegments * height / 640;
            return segs;
        }

        void captureFrame() throw() {
            if (frame_count_ >= 24*60) {
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

Render::Render() throw() {
}

Render::~Render() throw() {
}

Render *Render::create() throw() {
    auto impl = new(std::nothrow) RenderImpl;
    return impl;
}
