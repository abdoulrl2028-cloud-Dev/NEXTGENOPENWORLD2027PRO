#include "graphics/GltfLoader.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

namespace {

bool readFile(const char *path, std::vector<uint8_t> &out) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "cannot open " << path << "\n";
        return false;
    }
    input.seekg(0, std::ios::end);
    const std::streamoff length = input.tellg();
    if (length <= 0) {
        std::cerr << "empty file " << path << "\n";
        return false;
    }
    input.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(length));
    input.read(reinterpret_cast<char *>(out.data()), length);
    return input.good() || input.eof();
}

glm::vec3 transformPoint(const float *matrix, float x, float y, float z) {
    return glm::vec3(
            matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
            matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
            matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14]);
}

}  // namespace

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "app/src/main/assets/models/vera_costa_block.glb";
    std::vector<uint8_t> bytes;
    if (!readFile(path, bytes)) {
        return 1;
    }

    CpuModel model;
    std::string error;
    if (!GltfLoader::loadFromMemory(bytes.data(), bytes.size(), model, error)) {
        std::cerr << error << "\n";
        return 1;
    }

    if (model.primitives.size() < 8) {
        std::cerr << "expected several scene meshes, got " << model.primitives.size() << "\n";
        return 1;
    }

    bool sawTranslation = false;
    bool sawWater = false;
    glm::vec3 minimum(1e9f);
    glm::vec3 maximum(-1e9f);
    size_t triangles = 0;

    for (const CpuPrimitive &primitive : model.primitives) {
        if (primitive.vertices.empty() || primitive.indices.size() < 3 ||
            primitive.indices.size() % 3 != 0) {
            std::cerr << primitive.name << " has invalid geometry\n";
            return 1;
        }
        const float translationLength = std::sqrt(
                primitive.model[12] * primitive.model[12] +
                primitive.model[13] * primitive.model[13] +
                primitive.model[14] * primitive.model[14]);
        if (translationLength > 1.0f) {
            sawTranslation = true;
        }
        if (primitive.name.find("Water") != std::string::npos) {
            sawWater = true;
            if (primitive.baseColor[2] <= primitive.baseColor[0]) {
                std::cerr << "water material is not blue-dominant\n";
                return 1;
            }
        }
        for (uint32_t index : primitive.indices) {
            if (index >= primitive.vertices.size()) {
                std::cerr << primitive.name << " index out of range\n";
                return 1;
            }
        }
        for (const CpuVertex &vertex : primitive.vertices) {
            const glm::vec3 world = transformPoint(primitive.model, vertex.px, vertex.py, vertex.pz);
            minimum = glm::min(minimum, world);
            maximum = glm::max(maximum, world);
            const float normalLength = std::sqrt(
                    vertex.nx * vertex.nx + vertex.ny * vertex.ny + vertex.nz * vertex.nz);
            if (normalLength < 0.9f || normalLength > 1.1f) {
                std::cerr << primitive.name << " has a non-unit normal\n";
                return 1;
            }
        }
        triangles += primitive.indices.size() / 3;
    }

    const glm::vec3 extent = maximum - minimum;
    if (!sawTranslation || !sawWater || extent.x < 40.0f || extent.z < 40.0f) {
        std::cerr << "scene bounds or transforms are incomplete\n";
        return 1;
    }

    std::cout << "gltf_loader_test ok primitives=" << model.primitives.size()
              << " triangles=" << triangles
              << " extent=" << extent.x << "," << extent.y << "," << extent.z << "\n";
    return 0;
}
