#ifndef NEXTGEN_GRAPHICS_SCENESHADER_H
#define NEXTGEN_GRAPHICS_SCENESHADER_H

#include <string>

#include <glm/glm.hpp>

#include "GlslProgram.h"
#include "StaticMesh.h"

class SceneShader {
public:
    bool create(const std::string &vertexSource, const std::string &fragmentSource, std::string &error);
    void draw(
            const StaticMesh &mesh,
            const glm::mat4 &view,
            const glm::mat4 &projection,
            const glm::vec3 &cameraPosition) const;
    void draw(
            const StaticMesh &mesh,
            const glm::mat4 &model,
            const glm::mat4 &view,
            const glm::mat4 &projection,
            const glm::vec3 &cameraPosition) const;

private:
    GlslProgram program_;
    GLint model_ = -1;
    GLint view_ = -1;
    GLint projection_ = -1;
    GLint baseColor_ = -1;
    GLint metallic_ = -1;
    GLint roughness_ = -1;
    GLint lightDirection_ = -1;
    GLint cameraPosition_ = -1;
};

#endif
