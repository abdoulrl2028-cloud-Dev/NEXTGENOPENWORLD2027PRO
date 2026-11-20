#ifndef NEXTGEN_GRAPHICS_GLTFLOADER_H
#define NEXTGEN_GRAPHICS_GLTFLOADER_H

#include <cstdint>
#include <string>
#include <vector>

struct CpuVertex {
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

struct CpuPrimitive {
    std::string name;
    std::vector<CpuVertex> vertices;
    std::vector<uint32_t> indices;
    float model[16];
    float baseColor[4];
    float metallic;
    float roughness;
};

struct CpuModel {
    std::vector<CpuPrimitive> primitives;
};

class GltfLoader {
public:
    static bool loadFromMemory(
            const uint8_t *bytes,
            size_t size,
            CpuModel &outModel,
            std::string &error);
};

#endif
