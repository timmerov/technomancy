/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
opengl utilities.
**/

#include <GLES3/gl3.h>

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/opengl.h>


GLuint agm::gl::compileShader(
    GLenum type,
    const char *source
) noexcept {
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

GLuint agm::gl::linkProgram(
    GLuint vertex_shader,
    GLuint fragment_shader
) noexcept {
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
