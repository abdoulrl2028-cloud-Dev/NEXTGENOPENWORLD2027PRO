#ifndef NEXTGEN_UI_SCENEMANAGER_H
#define NEXTGEN_UI_SCENEMANAGER_H

class SceneManager {
public:
    enum class SceneId {
        MainMenu,
        Gameplay
    };

    void request(SceneId scene);
    void update(float deltaSeconds);
    SceneId active() const { return active_; }
    float fade() const { return fade_; }
    bool acceptsInput() const { return fade_ < 0.12f && !fadingOut_; }

private:
    SceneId active_ = SceneId::MainMenu;
    SceneId pending_ = SceneId::MainMenu;
    float fade_ = 0.0f;
    bool fadingOut_ = false;
    bool fadingIn_ = false;
};

#endif
