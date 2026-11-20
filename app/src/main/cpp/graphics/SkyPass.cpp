#include "SkyPass.h"

#include <GLES3/gl3.h>

SkyPass::~SkyPass() {
    release();
}

void SkyPass::release() {
    if (buffer_ != 0) {
        glDeleteBuffers(1, &buffer_);
        buffer_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
}

bool SkyPass::create(
        const std::string &vertexSource,
        const std::string &fragmentSource,
        std::string &error) {
    if (!program_.link(vertexSource, fragmentSource, error)) {
        return false;
    }
    const float triangle[] = {
            -1.0f, -1.0f,
            3.0f, -1.0f,
            -1.0f, 3.0f};
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &buffer_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, buffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangle), triangle, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);
    glBindVertexArray(0);
    return vao_ != 0;
}

void SkyPass::draw() const {
    if (vao_ == 0) {
        return;
    }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    program_.use();
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}
