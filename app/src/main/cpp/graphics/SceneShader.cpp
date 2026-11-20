#include "SceneShader.h"

#include <glm/gtc/type_ptr.hpp>

bool SceneShader::create(
        const std::string &vertexSource,
        const std::string &fragmentSource,
        std::string &error) {
    if (!program_.link(vertexSource, fragmentSource, error)) {
        return false;
    }
    model_ = program_.uniform("uModel");
    view_ = program_.uniform("uView");
    projection_ = program_.uniform("uProjection");
    baseColor_ = program_.uniform("uBaseColor");
    metallic_ = program_.uniform("uMetallic");
    roughness_ = program_.uniform("uRoughness");
    lightDirection_ = program_.uniform("uLightDirection");
    cameraPosition_ = program_.uniform("uCameraPosition");
    if (model_ < 0 || view_ < 0 || projection_ < 0 || baseColor_ < 0 || lightDirection_ < 0) {
        error = "scene shader is missing a required uniform";
        return false;
    }
    return true;
}

void SceneShader::draw(
        const StaticMesh &mesh,
        const glm::mat4 &view,
        const glm::mat4 &projection,
        const glm::vec3 &cameraPosition) const {
    draw(mesh, glm::make_mat4(mesh.modelMatrix()), view, projection, cameraPosition);
}

void SceneShader::draw(
        const StaticMesh &mesh,
        const glm::mat4 &model,
        const glm::mat4 &view,
        const glm::mat4 &projection,
        const glm::vec3 &cameraPosition) const {
    program_.use();
    glUniformMatrix4fv(model_, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(view_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform4fv(baseColor_, 1, mesh.baseColor());
    if (metallic_ >= 0) {
        glUniform1f(metallic_, mesh.metallic());
    }
    if (roughness_ >= 0) {
        glUniform1f(roughness_, mesh.roughness());
    }
    const glm::vec3 light = glm::normalize(glm::vec3(0.35f, 0.86f, 0.28f));
    glUniform3fv(lightDirection_, 1, glm::value_ptr(light));
    if (cameraPosition_ >= 0) {
        glUniform3fv(cameraPosition_, 1, glm::value_ptr(cameraPosition));
    }
    mesh.draw();
}
