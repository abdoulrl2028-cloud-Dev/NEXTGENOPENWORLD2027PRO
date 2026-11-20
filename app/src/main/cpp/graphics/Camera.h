#ifndef NEXTGEN_GRAPHICS_CAMERA_H
#define NEXTGEN_GRAPHICS_CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    void addOrbit(float deltaXPixels, float deltaYPixels);
    void multiplyDistance(float ratio);
    void update(float deltaSeconds);
    void setLookSensitivity(float sensitivity);
    void setInvertLook(bool invert);
    void setFarPlane(float farPlane);

    glm::mat4 view() const;
    glm::mat4 projection(float aspect) const;
    glm::vec3 position() const;
    glm::vec3 target() const { return target_; }

private:
    glm::vec3 target_{0.0f, 2.2f, 4.0f};
    float yaw_ = 0.85f;
    float pitch_ = 0.42f;
    float distance_ = 28.0f;
    float targetYaw_ = 0.85f;
    float targetPitch_ = 0.42f;
    float targetDistance_ = 28.0f;
    float lookSensitivity_ = 1.0f;
    float farPlane_ = 300.0f;
    bool invertLook_ = false;
};

#endif
