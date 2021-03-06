/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

#pragma once

/**
opengl utilities.
**/

#include <GL/gl.h>

namespace agm {
    namespace gl {
        GLuint compileShader(GLenum type, const char *source) noexcept;
        GLuint linkProgram(GLuint vertex_shader, GLuint fragment_shader) noexcept;
    }
}
