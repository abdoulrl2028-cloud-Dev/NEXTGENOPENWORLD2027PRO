#include "SceneManager.h"

void SceneManager::request(SceneId scene) {
    if (scene == active_ || fadingOut_ || fadingIn_) {
        return;
    }
    pending_ = scene;
    fadingOut_ = true;
}

void SceneManager::update(float deltaSeconds) {
    const float step = deltaSeconds / 0.45f;
    if (fadingOut_) {
        fade_ += step;
        if (fade_ >= 1.0f) {
            fade_ = 1.0f;
            active_ = pending_;
            fadingOut_ = false;
            fadingIn_ = true;
        }
    } else if (fadingIn_) {
        fade_ -= step;
        if (fade_ <= 0.0f) {
            fade_ = 0.0f;
            fadingIn_ = false;
        }
    }
}
