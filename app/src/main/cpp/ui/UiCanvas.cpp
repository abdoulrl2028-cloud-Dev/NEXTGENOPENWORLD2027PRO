#include "UiCanvas.h"

#include "AndroidOut.h"

namespace {

GLuint compile(GLenum type, const std::string &source, std::string &error) {
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
        error.assign(static_cast<size_t>(logLength > 0 ? logLength : 1), '\0');
        if (logLength > 0) {
            glGetShaderInfoLog(shader, logLength, nullptr, error.data());
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

}  // namespace

bool UiCanvas::create(const std::string &vertexSource, const std::string &fragmentSource, std::string &error) {
    destroy();
    const GLuint vertex = compile(GL_VERTEX_SHADER, vertexSource, error);
    if (vertex == 0) {
        aout << "[NG] ui vertex shader: " << error << std::endl;
        return false;
    }
    const GLuint fragment = compile(GL_FRAGMENT_SHADER, fragmentSource, error);
    if (fragment == 0) {
        aout << "[NG] ui fragment shader: " << error << std::endl;
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
        error.assign(static_cast<size_t>(logLength > 0 ? logLength : 1), '\0');
        if (logLength > 0) {
            glGetProgramInfoLog(program_, logLength, nullptr, error.data());
        }
        destroy();
        return false;
    }
    resolution_ = glGetUniformLocation(program_, "uResolution");
    color_ = glGetUniformLocation(program_, "uColor");

    const unsigned char whitePixel[4] = {255, 255, 255, 255};
    glGenTextures(1, &white_);
    glBindTexture(GL_TEXTURE_2D, white_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &buffer_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<void *>(sizeof(float) * 2));
    glBindVertexArray(0);
    return program_ != 0 && vao_ != 0 && white_ != 0;
}

void UiCanvas::destroy() {
    if (buffer_ != 0) {
        glDeleteBuffers(1, &buffer_);
        buffer_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    if (white_ != 0) {
        glDeleteTextures(1, &white_);
        white_ = 0;
    }
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
}

void UiCanvas::begin(int screenWidth, int screenHeight) {
    width_ = screenWidth;
    height_ = screenHeight;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(program_);
    glUniform2f(resolution_, static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    glBindVertexArray(vao_);
}

void UiCanvas::end() {
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_BLEND);
}

void UiCanvas::drawSolid(float x, float y, float w, float h, float red, float green, float blue, float alpha) {
    drawQuad(x, y, w, h, white_, 0.0f, 0.0f, 1.0f, 1.0f, red, green, blue, alpha);
}

void UiCanvas::drawTextured(
        float x,
        float y,
        float w,
        float h,
        GLuint texture,
        float u0,
        float v0,
        float u1,
        float v1,
        float red,
        float green,
        float blue,
        float alpha) {
    drawQuad(x, y, w, h, texture, u0, v0, u1, v1, red, green, blue, alpha);
}

void UiCanvas::drawQuad(
        float x,
        float y,
        float w,
        float h,
        GLuint texture,
        float u0,
        float v0,
        float u1,
        float v1,
        float red,
        float green,
        float blue,
        float alpha) {
    const float vertices[] = {
            x, y, u0, v0,
            x + w, y, u1, v0,
            x + w, y + h, u1, v1,
            x, y, u0, v0,
            x + w, y + h, u1, v1,
            x, y + h, u0, v1};
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform4f(color_, red, green, blue, alpha);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
