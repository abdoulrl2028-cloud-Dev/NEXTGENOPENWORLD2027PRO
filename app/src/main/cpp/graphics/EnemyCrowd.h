#ifndef NEXTGEN_GRAPHICS_ENEMYCROWD_H
#define NEXTGEN_GRAPHICS_ENEMYCROWD_H

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "StaticMesh.h"

class SceneShader;
struct AAssetManager;

class EnemyCrowd {
public:
    bool load(AAssetManager *assets, std::string &error);
    void clear();
    void update(float deltaSeconds, const glm::vec3 &focus);
    void draw(
            const SceneShader &shader,
            const glm::mat4 &view,
            const glm::mat4 &projection,
            const glm::vec3 &cameraPosition) const;

    int count() const { return static_cast<int>(agents_.size()); }
    bool ready() const { return !parts_.empty() && !agents_.empty(); }

private:
    struct Agent {
        glm::vec3 position{0.0f};
        float yaw = 0.0f;
        float phase = 0.0f;
        int waypoint = 0;
        bool alert = false;
        float speed = 0.0f;
    };

    const StaticMesh *findPart(const std::string &name) const;
    void drawPart(const SceneShader &shader, const std::string &name, const glm::mat4 &model,
                  const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPosition) const;

    std::unordered_map<std::string, StaticMesh> parts_;
    std::vector<Agent> agents_;
};

#endif
