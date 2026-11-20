#include "GlslProgram.h"

#include <algorithm>

#include "AndroidOut.h"

GlslProgram::GlslProgram(GlslProgram &&other) noexcept : program_(other.program_) {
    other.program_ = 0;
}

GlslProgram &GlslProgram::operator=(GlslProgram &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    release();
    program_ = other.program_;
    other.program_ = 0;
    return *this;
}

GlslProgram::~GlslProgram() {
    release();
}

void GlslProgram::release() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

GLuint GlslProgram::compile(GLenum type, const std::string &source, std::string &error) {
    const GLuint shader = glCreateShader(type);
    const char *raw = source.c_str();
    const GLint length = static_cast<GLint>(source.size());
    glShaderSource(shader, 1, &raw, &length);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<size_t>(std::max(logLength, 1)), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        error = log;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool GlslProgram::link(
        const std::string &vertexSource,
        const std::string &fragmentSource,
        std::string &error) {
    release();
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource, error);
    if (vertex == 0) {
        aout << "[NG] vertex shader: " << error << std::endl;
        return false;
    }
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource, error);
    if (fragment == 0) {
        aout << "[NG] fragment shader: " << error << std::endl;
        glDeleteShader(vertex);
        return false;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vertex);
    glAttachShader(program_, fragment);
    glLinkProgram(program_);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &logLength);
        error.assign(static_cast<size_t>(std::max(logLength, 1)), '\0');
        glGetProgramInfoLog(program_, logLength, nullptr, error.data());
        aout << "[NG] program link: " << error << std::endl;
        release();
        return false;
    }
    return true;
}

void GlslProgram::use() const {
    glUseProgram(program_);
}

GLint GlslProgram::uniform(const char *name) const {
    return glGetUniformLocation(program_, name);
}
