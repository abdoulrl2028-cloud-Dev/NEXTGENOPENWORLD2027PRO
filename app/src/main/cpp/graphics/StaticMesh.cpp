#include "StaticMesh.h"

#include <cstddef>
#include <cstring>

StaticMesh::StaticMesh(StaticMesh &&other) noexcept
        : vao_(other.vao_),
          vertexBuffer_(other.vertexBuffer_),
          indexBuffer_(other.indexBuffer_),
          indexCount_(other.indexCount_),
          metallic_(other.metallic_),
          roughness_(other.roughness_),
          name_(std::move(other.name_)) {
    std::memcpy(model_, other.model_, sizeof(model_));
    std::memcpy(baseColor_, other.baseColor_, sizeof(baseColor_));
    other.vao_ = 0;
    other.vertexBuffer_ = 0;
    other.indexBuffer_ = 0;
    other.indexCount_ = 0;
}

StaticMesh &StaticMesh::operator=(StaticMesh &&other) noexcept {
    if (this == &other) {
        return *this;
    }
    release();
    vao_ = other.vao_;
    vertexBuffer_ = other.vertexBuffer_;
    indexBuffer_ = other.indexBuffer_;
    indexCount_ = other.indexCount_;
    metallic_ = other.metallic_;
    roughness_ = other.roughness_;
    name_ = std::move(other.name_);
    std::memcpy(model_, other.model_, sizeof(model_));
    std::memcpy(baseColor_, other.baseColor_, sizeof(baseColor_));
    other.vao_ = 0;
    other.vertexBuffer_ = 0;
    other.indexBuffer_ = 0;
    other.indexCount_ = 0;
    return *this;
}

StaticMesh::~StaticMesh() {
    release();
}

void StaticMesh::release() {
    if (indexBuffer_ != 0) {
        glDeleteBuffers(1, &indexBuffer_);
        indexBuffer_ = 0;
    }
    if (vertexBuffer_ != 0) {
        glDeleteBuffers(1, &vertexBuffer_);
        vertexBuffer_ = 0;
    }
    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }
    indexCount_ = 0;
}

bool StaticMesh::upload(const CpuPrimitive &primitive) {
    release();
    if (primitive.vertices.empty() || primitive.indices.empty()) {
        return false;
    }

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(primitive.vertices.size() * sizeof(CpuVertex)),
            primitive.vertices.data(),
            GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(primitive.indices.size() * sizeof(uint32_t)),
            primitive.indices.data(),
            GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CpuVertex), reinterpret_cast<void *>(offsetof(CpuVertex, px)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CpuVertex), reinterpret_cast<void *>(offsetof(CpuVertex, nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CpuVertex), reinterpret_cast<void *>(offsetof(CpuVertex, u)));
    glBindVertexArray(0);

    std::memcpy(model_, primitive.model, sizeof(model_));
    std::memcpy(baseColor_, primitive.baseColor, sizeof(baseColor_));
    metallic_ = primitive.metallic;
    roughness_ = primitive.roughness;
    indexCount_ = static_cast<GLsizei>(primitive.indices.size());
    name_ = primitive.name;
    return vao_ != 0 && indexCount_ > 0;
}

void StaticMesh::draw() const {
    if (vao_ == 0 || indexCount_ <= 0) {
        return;
    }
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
