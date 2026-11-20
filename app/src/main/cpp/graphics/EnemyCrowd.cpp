#include "EnemyCrowd.h"

#include <android/asset_manager.h>

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "SceneShader.h"
#include "core/AssetFile.h"
#include "GltfLoader.h"

namespace {

constexpr const char *kEnemyAsset = "models/enemy_guard.glb";
constexpr float kGroundY = 0.16f;
constexpr float kAlertDistance = 11.0f;
constexpr float kStopDistance = 1.8f;
constexpr float kPatrolSpeed = 1.45f;
constexpr float kChaseSpeed = 2.15f;

const glm::vec3 kRoute[] = {
        {7.4f, kGroundY, 3.5f},
        {6.2f, kGroundY, 14.0f},
        {10.8f, kGroundY, 16.5f},
        {12.4f, kGroundY, 2.0f},
        {8.6f, kGroundY, -8.0f},
        {4.2f, kGroundY, -4.0f},
};

glm::mat4 rotationX(float radians) {
    return glm::rotate(glm::mat4(1.0f), radians, glm::vec3(1.0f, 0.0f, 0.0f));
}

float wrapPi(float radians) {
    while (radians > 3.14159265f) {
        radians -= 6.2831853f;
    }
    while (radians < -3.14159265f) {
        radians += 6.2831853f;
    }
    return radians;
}

glm::vec3 clampFocus(glm::vec3 focus) {
    focus.x = std::clamp(focus.x, -2.0f, 28.0f);
    focus.y = kGroundY;
    focus.z = std::clamp(focus.z, -30.0f, 32.0f);
    return focus;
}

}  // namespace

bool EnemyCrowd::load(AAssetManager *assets, std::string &error) {
    clear();
    std::vector<uint8_t> bytes;
    if (!AssetFile::readBinary(assets, kEnemyAsset, bytes, error)) {
        return false;
    }
    CpuModel model;
    if (!GltfLoader::loadFromMemory(bytes.data(), bytes.size(), model, error)) {
        return false;
    }
    for (const CpuPrimitive &primitive : model.primitives) {
        StaticMesh mesh;
        if (!mesh.upload(primitive)) {
            error = "failed to upload " + primitive.name;
            clear();
            return false;
        }
        parts_.emplace(primitive.name, std::move(mesh));
    }
    if (findPart("hips") == nullptr || findPart("head") == nullptr || findPart("thigh_l") == nullptr) {
        error = "enemy_guard.glb is missing a body part";
        clear();
        return false;
    }

    const int routeCount = static_cast<int>(sizeof(kRoute) / sizeof(kRoute[0]));
    agents_.resize(5);
    for (int index = 0; index < static_cast<int>(agents_.size()); ++index) {
        Agent &agent = agents_[static_cast<size_t>(index)];
        agent.position = kRoute[index % routeCount];
        agent.waypoint = (index + 1) % routeCount;
        agent.phase = static_cast<float>(index) * 1.3f;
        agent.yaw = 0.4f * static_cast<float>(index);
    }
    return true;
}

void EnemyCrowd::clear() {
    parts_.clear();
    agents_.clear();
}

const StaticMesh *EnemyCrowd::findPart(const std::string &name) const {
    const auto found = parts_.find(name);
    if (found == parts_.end()) {
        return nullptr;
    }
    return &found->second;
}

void EnemyCrowd::update(float deltaSeconds, const glm::vec3 &focus) {
    if (!ready() || deltaSeconds <= 0.0f) {
        return;
    }
    const float step = std::min(deltaSeconds, 0.05f);
    const glm::vec3 chasePoint = clampFocus(focus);
    const int routeCount = static_cast<int>(sizeof(kRoute) / sizeof(kRoute[0]));
    for (Agent &agent : agents_) {
        const glm::vec3 toFocus = chasePoint - agent.position;
        const float focusDistance = std::hypot(toFocus.x, toFocus.z);
        agent.alert = focusDistance < kAlertDistance;
        glm::vec3 goal = agent.alert ? chasePoint : kRoute[agent.waypoint];
        glm::vec3 offset = goal - agent.position;
        offset.y = 0.0f;
        const float distance = glm::length(offset);
        const float speed = agent.alert ? kChaseSpeed : kPatrolSpeed;
        const bool arrived = distance < (agent.alert ? kStopDistance : 0.35f);
        if (!agent.alert && arrived) {
            agent.waypoint = (agent.waypoint + 1) % routeCount;
        }
        if (!arrived && distance > 0.001f) {
            const glm::vec3 direction = offset / distance;
            const float travel = std::min(distance, speed * step);
            agent.position += direction * travel;
            agent.position.y = kGroundY;
            const float desiredYaw = std::atan2(direction.x, direction.z);
            agent.yaw += wrapPi(desiredYaw - agent.yaw) * std::min(1.0f, step * 6.0f);
            agent.speed = speed;
            agent.phase += travel * 5.5f;
        } else {
            agent.speed = 0.0f;
            agent.phase += step * 1.4f;
        }
    }
}

void EnemyCrowd::drawPart(
        const SceneShader &shader,
        const std::string &name,
        const glm::mat4 &model,
        const glm::mat4 &view,
        const glm::mat4 &projection,
        const glm::vec3 &cameraPosition) const {
    const StaticMesh *part = findPart(name);
    if (part == nullptr) {
        return;
    }
    shader.draw(*part, model, view, projection, cameraPosition);
}

void EnemyCrowd::draw(
        const SceneShader &shader,
        const glm::mat4 &view,
        const glm::mat4 &projection,
        const glm::vec3 &cameraPosition) const {
    for (const Agent &agent : agents_) {
        const float moving = std::clamp(agent.speed / kPatrolSpeed, 0.0f, 1.0f);
        const float swing = std::sin(agent.phase) * (0.08f + 0.52f * moving);
        const float thighL = swing;
        const float thighR = -swing;
        const float kneeL = std::max(0.0f, -thighL) * 1.25f;
        const float kneeR = std::max(0.0f, -thighR) * 1.25f;
        const float armL = -thighL;
        const float armR = -thighR;
        const float elbowL = 0.18f + std::max(0.0f, armL) * 0.45f;
        const float elbowR = 0.18f + std::max(0.0f, armR) * 0.45f;
        const float lean = std::sin(agent.phase) * 0.035f * moving;

        const glm::mat4 root = glm::translate(glm::mat4(1.0f), agent.position) *
                               glm::rotate(glm::mat4(1.0f), agent.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 hips = root * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.96f, 0.0f));
        const glm::mat4 torso = hips * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.04f, 0.0f)) * rotationX(lean);
        const glm::mat4 head = torso * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.46f, 0.02f));
        const glm::mat4 shoulderL = torso * glm::translate(glm::mat4(1.0f), glm::vec3(-0.22f, 0.40f, 0.0f)) * rotationX(armL);
        const glm::mat4 shoulderR = torso * glm::translate(glm::mat4(1.0f), glm::vec3(0.22f, 0.40f, 0.0f)) * rotationX(armR);
        const glm::mat4 elbowLJoint = shoulderL * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.30f, 0.0f)) * rotationX(elbowL);
        const glm::mat4 elbowRJoint = shoulderR * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.30f, 0.0f)) * rotationX(elbowR);
        const glm::mat4 hipL = hips * glm::translate(glm::mat4(1.0f), glm::vec3(-0.09f, -0.04f, 0.0f)) * rotationX(thighL);
        const glm::mat4 hipR = hips * glm::translate(glm::mat4(1.0f), glm::vec3(0.09f, -0.04f, 0.0f)) * rotationX(thighR);
        const glm::mat4 kneeLJoint = hipL * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.46f, 0.0f)) * rotationX(kneeL);
        const glm::mat4 kneeRJoint = hipR * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.46f, 0.0f)) * rotationX(kneeR);

        drawPart(shader, "hips", hips, view, projection, cameraPosition);
        drawPart(shader, "torso", torso, view, projection, cameraPosition);
        drawPart(shader, "torso_vest", torso, view, projection, cameraPosition);
        drawPart(shader, "head", head, view, projection, cameraPosition);
        drawPart(shader, "head_hair", head, view, projection, cameraPosition);
        drawPart(shader, "head_eyes", head, view, projection, cameraPosition);
        drawPart(shader, "upper_arm_l", shoulderL, view, projection, cameraPosition);
        drawPart(shader, "upper_arm_r", shoulderR, view, projection, cameraPosition);
        drawPart(shader, "forearm_l", elbowLJoint, view, projection, cameraPosition);
        drawPart(shader, "forearm_r", elbowRJoint, view, projection, cameraPosition);
        drawPart(shader, "thigh_l", hipL, view, projection, cameraPosition);
        drawPart(shader, "thigh_r", hipR, view, projection, cameraPosition);
        drawPart(shader, "shin_l", kneeLJoint, view, projection, cameraPosition);
        drawPart(shader, "shin_r", kneeRJoint, view, projection, cameraPosition);
        drawPart(shader, "boot_l", kneeLJoint, view, projection, cameraPosition);
        drawPart(shader, "boot_r", kneeRJoint, view, projection, cameraPosition);
    }
}
