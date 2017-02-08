/*
Copyright (C) 2012-2016 tim cotter. All rights reserved.
*/

#pragma once

/**
opengl utilities.
**/

#include <GL/gl.h>

namespace agm {
    namespace gl {
        GLuint compileShader(GLenum type, const char *source) throw();
        GLuint linkProgram(GLuint vertex_shader, GLuint fragment_shader) throw();
    }
}
