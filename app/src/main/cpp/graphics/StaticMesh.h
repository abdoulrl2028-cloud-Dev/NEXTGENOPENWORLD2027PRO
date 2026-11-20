#ifndef NEXTGEN_GRAPHICS_STATICMESH_H
#define NEXTGEN_GRAPHICS_STATICMESH_H

#include <GLES3/gl3.h>

#include "GltfLoader.h"

class StaticMesh {
public:
    StaticMesh() = default;
    StaticMesh(const StaticMesh &) = delete;
    StaticMesh &operator=(const StaticMesh &) = delete;
    StaticMesh(StaticMesh &&other) noexcept;
    StaticMesh &operator=(StaticMesh &&other) noexcept;
    ~StaticMesh();

    bool upload(const CpuPrimitive &primitive);
    void draw() const;

    const float *modelMatrix() const { return model_; }
    const float *baseColor() const { return baseColor_; }
    float metallic() const { return metallic_; }
    float roughness() const { return roughness_; }
    const std::string &name() const { return name_; }

private:
    void release();

    GLuint vao_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLsizei indexCount_ = 0;
    float model_[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
    float baseColor_[4] = {1, 1, 1, 1};
    float metallic_ = 0;
    float roughness_ = 1;
    std::string name_;
};

#endif
