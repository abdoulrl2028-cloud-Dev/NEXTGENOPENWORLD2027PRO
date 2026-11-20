#ifndef NEXTGEN_GRAPHICS_GLSLPROGRAM_H
#define NEXTGEN_GRAPHICS_GLSLPROGRAM_H

#include <GLES3/gl3.h>

#include <string>

class GlslProgram {
public:
    GlslProgram() = default;
    GlslProgram(const GlslProgram &) = delete;
    GlslProgram &operator=(const GlslProgram &) = delete;
    GlslProgram(GlslProgram &&other) noexcept;
    GlslProgram &operator=(GlslProgram &&other) noexcept;
    ~GlslProgram();

    bool link(const std::string &vertexSource, const std::string &fragmentSource, std::string &error);
    void use() const;
    GLint uniform(const char *name) const;
    GLuint id() const { return program_; }

private:
    void release();
    static GLuint compile(GLenum type, const std::string &source, std::string &error);

    GLuint program_ = 0;
};

#endif
