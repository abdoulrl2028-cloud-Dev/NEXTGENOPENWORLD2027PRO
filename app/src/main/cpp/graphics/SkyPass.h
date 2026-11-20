#ifndef NEXTGEN_GRAPHICS_SKYPASS_H
#define NEXTGEN_GRAPHICS_SKYPASS_H

#include <string>

#include "GlslProgram.h"

class SkyPass {
public:
    SkyPass() = default;
    SkyPass(const SkyPass &) = delete;
    SkyPass &operator=(const SkyPass &) = delete;
    ~SkyPass();

    bool create(const std::string &vertexSource, const std::string &fragmentSource, std::string &error);
    void draw() const;

private:
    void release();

    GlslProgram program_;
    GLuint vao_ = 0;
    GLuint buffer_ = 0;
};

#endif
