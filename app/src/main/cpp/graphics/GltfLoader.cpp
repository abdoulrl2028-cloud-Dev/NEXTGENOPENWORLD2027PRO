#include "GltfLoader.h"

#include <cstring>

#include <glm/gtc/type_ptr.hpp>

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

namespace {

const char *resultName(cgltf_result result) {
    switch (result) {
        case cgltf_result_success:
            return "success";
        case cgltf_result_data_too_short:
            return "data too short";
        case cgltf_result_unknown_format:
            return "unknown format";
        case cgltf_result_invalid_json:
            return "invalid json";
        case cgltf_result_invalid_gltf:
            return "invalid gltf";
        case cgltf_result_invalid_options:
            return "invalid options";
        case cgltf_result_file_not_found:
            return "file not found";
        case cgltf_result_io_error:
            return "io error";
        case cgltf_result_out_of_memory:
            return "out of memory";
        case cgltf_result_legacy_gltf:
            return "legacy gltf";
        default:
            return "gltf error";
    }
}

glm::mat4 worldMatrix(const cgltf_node *node) {
    float raw[16];
    cgltf_node_transform_world(node, raw);
    return glm::make_mat4(raw);
}

void storeMatrix(const glm::mat4 &matrix, float out[16]) {
    std::memcpy(out, glm::value_ptr(matrix), sizeof(float) * 16);
}

bool readPrimitive(
        const cgltf_primitive *primitive,
        const glm::mat4 &model,
        const char *name,
        CpuPrimitive &out,
        std::string &error) {
    if (primitive->type != cgltf_primitive_type_triangles) {
        error = std::string(name) + " uses a primitive mode other than triangles";
        return false;
    }

    const cgltf_accessor *positions = nullptr;
    const cgltf_accessor *normals = nullptr;
    const cgltf_accessor *uvs = nullptr;
    for (cgltf_size attributeIndex = 0; attributeIndex < primitive->attributes_count; ++attributeIndex) {
        const cgltf_attribute &attribute = primitive->attributes[attributeIndex];
        if (attribute.index != 0) {
            continue;
        }
        if (attribute.type == cgltf_attribute_type_position) {
            positions = attribute.data;
        } else if (attribute.type == cgltf_attribute_type_normal) {
            normals = attribute.data;
        } else if (attribute.type == cgltf_attribute_type_texcoord) {
            uvs = attribute.data;
        }
    }

    if (positions == nullptr || positions->count == 0) {
        error = std::string(name) + " has no positions";
        return false;
    }

    out.name = name;
    out.vertices.resize(positions->count);
    storeMatrix(model, out.model);
    out.baseColor[0] = 0.7f;
    out.baseColor[1] = 0.7f;
    out.baseColor[2] = 0.7f;
    out.baseColor[3] = 1.0f;
    out.metallic = 0.0f;
    out.roughness = 0.8f;
    if (primitive->material != nullptr) {
        const cgltf_pbr_metallic_roughness &pbr = primitive->material->pbr_metallic_roughness;
        std::memcpy(out.baseColor, pbr.base_color_factor, sizeof(float) * 4);
        out.metallic = pbr.metallic_factor;
        out.roughness = pbr.roughness_factor;
    }

    for (cgltf_size vertexIndex = 0; vertexIndex < positions->count; ++vertexIndex) {
        float position[3] = {0.0f, 0.0f, 0.0f};
        if (!cgltf_accessor_read_float(positions, vertexIndex, position, 3)) {
            error = std::string(name) + " position read failed";
            return false;
        }
        CpuVertex &vertex = out.vertices[vertexIndex];
        vertex.px = position[0];
        vertex.py = position[1];
        vertex.pz = position[2];
        vertex.nx = 0.0f;
        vertex.ny = 1.0f;
        vertex.nz = 0.0f;
        vertex.u = 0.0f;
        vertex.v = 0.0f;
        if (normals != nullptr) {
            float normal[3] = {0.0f, 1.0f, 0.0f};
            if (!cgltf_accessor_read_float(normals, vertexIndex, normal, 3)) {
                error = std::string(name) + " normal read failed";
                return false;
            }
            vertex.nx = normal[0];
            vertex.ny = normal[1];
            vertex.nz = normal[2];
        }
        if (uvs != nullptr) {
            float uv[2] = {0.0f, 0.0f};
            if (!cgltf_accessor_read_float(uvs, vertexIndex, uv, 2)) {
                error = std::string(name) + " uv read failed";
                return false;
            }
            vertex.u = uv[0];
            vertex.v = uv[1];
        }
    }

    if (primitive->indices == nullptr) {
        error = std::string(name) + " has no indices";
        return false;
    }
    out.indices.resize(primitive->indices->count);
    for (cgltf_size index = 0; index < primitive->indices->count; ++index) {
        const cgltf_size value = cgltf_accessor_read_index(primitive->indices, index);
        if (value >= out.vertices.size()) {
            error = std::string(name) + " index is out of range";
            return false;
        }
        out.indices[index] = static_cast<uint32_t>(value);
    }

    if (normals == nullptr) {
        std::vector<glm::vec3> accum(out.vertices.size(), glm::vec3(0.0f));
        for (size_t index = 0; index + 2 < out.indices.size(); index += 3) {
            const CpuVertex &v0 = out.vertices[out.indices[index]];
            const CpuVertex &v1 = out.vertices[out.indices[index + 1]];
            const CpuVertex &v2 = out.vertices[out.indices[index + 2]];
            const glm::vec3 edge1(v1.px - v0.px, v1.py - v0.py, v1.pz - v0.pz);
            const glm::vec3 edge2(v2.px - v0.px, v2.py - v0.py, v2.pz - v0.pz);
            const glm::vec3 face = glm::cross(edge1, edge2);
            accum[out.indices[index]] += face;
            accum[out.indices[index + 1]] += face;
            accum[out.indices[index + 2]] += face;
        }
        for (size_t vertexIndex = 0; vertexIndex < out.vertices.size(); ++vertexIndex) {
            glm::vec3 normal = accum[vertexIndex];
            if (glm::dot(normal, normal) < 1e-8f) {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                normal = glm::normalize(normal);
            }
            out.vertices[vertexIndex].nx = normal.x;
            out.vertices[vertexIndex].ny = normal.y;
            out.vertices[vertexIndex].nz = normal.z;
        }
    }

    return true;
}

}  // namespace

bool GltfLoader::loadFromMemory(
        const uint8_t *bytes,
        size_t size,
        CpuModel &outModel,
        std::string &error) {
    outModel.primitives.clear();
    if (bytes == nullptr || size == 0) {
        error = "glTF buffer is empty";
        return false;
    }

    cgltf_options options = {};
    cgltf_data *gltf = nullptr;
    const cgltf_result parsed = cgltf_parse(&options, bytes, size, &gltf);
    if (parsed != cgltf_result_success || gltf == nullptr) {
        error = std::string("cgltf_parse failed: ") + resultName(parsed);
        return false;
    }

    const cgltf_result buffers = cgltf_load_buffers(&options, gltf, "memory.glb");
    if (buffers != cgltf_result_success) {
        error = std::string("cgltf_load_buffers failed: ") + resultName(buffers);
        cgltf_free(gltf);
        return false;
    }

    const cgltf_result valid = cgltf_validate(gltf);
    if (valid != cgltf_result_success) {
        error = std::string("cgltf_validate failed: ") + resultName(valid);
        cgltf_free(gltf);
        return false;
    }

    for (cgltf_size nodeIndex = 0; nodeIndex < gltf->nodes_count; ++nodeIndex) {
        const cgltf_node *node = &gltf->nodes[nodeIndex];
        if (node->mesh == nullptr) {
            continue;
        }
        const glm::mat4 model = worldMatrix(node);
        const char *nodeName = node->name != nullptr ? node->name : "node";
        for (cgltf_size primitiveIndex = 0; primitiveIndex < node->mesh->primitives_count; ++primitiveIndex) {
            CpuPrimitive primitive;
            std::string primitiveError;
            if (!readPrimitive(
                    &node->mesh->primitives[primitiveIndex],
                    model,
                    nodeName,
                    primitive,
                    primitiveError)) {
                error = primitiveError;
                cgltf_free(gltf);
                outModel.primitives.clear();
                return false;
            }
            outModel.primitives.push_back(std::move(primitive));
        }
    }

    cgltf_free(gltf);
    if (outModel.primitives.empty()) {
        error = "glTF contains no mesh nodes";
        return false;
    }
    return true;
}
