#include "Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

void Camera::addOrbit(float deltaXPixels, float deltaYPixels) {
    const float ySign = invertLook_ ? -1.0f : 1.0f;
    targetYaw_ -= deltaXPixels * 0.005f * lookSensitivity_;
    targetPitch_ -= deltaYPixels * 0.004f * lookSensitivity_ * ySign;
    targetPitch_ = std::clamp(targetPitch_, 0.12f, 1.20f);
}

void Camera::setLookSensitivity(float sensitivity) {
    lookSensitivity_ = std::clamp(sensitivity, 0.4f, 2.0f);
}

void Camera::setInvertLook(bool invert) {
    invertLook_ = invert;
}

void Camera::setFarPlane(float farPlane) {
    farPlane_ = std::clamp(farPlane, 20.0f, 400.0f);
}

void Camera::multiplyDistance(float ratio) {
    if (!(ratio > 0.0f)) {
        return;
    }
    targetDistance_ *= ratio;
    targetDistance_ = std::clamp(targetDistance_, 6.0f, 80.0f);
}

void Camera::update(float deltaSeconds) {
    const float blend = 1.0f - std::exp(-8.0f * std::max(deltaSeconds, 0.0f));
    yaw_ += (targetYaw_ - yaw_) * blend;
    pitch_ += (targetPitch_ - pitch_) * blend;
    distance_ += (targetDistance_ - distance_) * blend;
}

glm::vec3 Camera::position() const {
    const float cosinePitch = std::cos(pitch_);
    const float sinePitch = std::sin(pitch_);
    const float cosineYaw = std::cos(yaw_);
    const float sineYaw = std::sin(yaw_);
    return target_ + glm::vec3(
            distance_ * cosinePitch * sineYaw,
            distance_ * sinePitch,
            distance_ * cosinePitch * cosineYaw);
}

glm::mat4 Camera::view() const {
    return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projection(float aspect) const {
    const float safeAspect = aspect > 0.01f ? aspect : 1.0f;
    return glm::perspective(glm::radians(55.0f), safeAspect, 0.15f, farPlane_);
}
