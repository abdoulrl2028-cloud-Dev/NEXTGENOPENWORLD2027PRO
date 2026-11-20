#ifndef NEXTGEN_RENDERER_H
#define NEXTGEN_RENDERER_H

#include <EGL/egl.h>
#include <game-activity/GameActivity.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "graphics/Camera.h"
#include "graphics/EnemyCrowd.h"
#include "graphics/SceneShader.h"
#include "graphics/SkyPass.h"
#include "graphics/StaticMesh.h"
#include "ui/GameSettings.h"
#include "ui/MenuContext.h"

struct android_app;
class MainMenuScene;
class MenuAudio;
class SceneManager;
class UiCanvas;

class Renderer : public MenuContext {
public:
    explicit Renderer(android_app *app);
    ~Renderer() override;

    void handleInput();
    void update(float deltaSeconds);
    void render();

    int screenWidth() const override;
    int screenHeight() const override;
    AAssetManager *assets() const override;
    const GameSettings &settings() const override;
    void applySettings(const GameSettings &settings) override;
    bool hasCampaignSave() const override;
    void startNewGame() override;
    void continueGame() override;
    void quitGame() override;

private:
    struct PointerSample {
        float x;
        float y;
    };

    void initRenderer();
    void updateRenderArea();
    bool loadScene();
    void renderWorld();
    void updateCameraFromPointers(uint32_t pointerCount, const GameActivityPointerAxes *pointers);
    void routeMenuPointer(int action, float x, float y);
    void loadSettings();
    bool saveSettings() const;
    void applyRuntimeSettings();
    std::string internalPath(const char *name) const;
    float qualityFarPlane() const;
    float qualityViewportScale() const;

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    Camera camera_;
    EnemyCrowd enemies_;
    std::unique_ptr<SceneShader> sceneShader_;
    std::unique_ptr<SkyPass> skyPass_;
    std::vector<StaticMesh> meshes_;
    std::unordered_map<int32_t, PointerSample> pointers_;
    bool orbitPrimed_ = false;
    bool pinchPrimed_ = false;
    float pinchDistance_ = 0.0f;
    bool sceneReady_ = false;
    std::string sceneStatus_;
    float fpsAccum_ = 0.0f;
    int frameCount_ = 0;

    GameSettings settings_;
    std::unique_ptr<UiCanvas> canvas_;
    std::unique_ptr<MainMenuScene> menu_;
    std::unique_ptr<SceneManager> scenes_;
    std::unique_ptr<MenuAudio> audio_;
    float musicFade_ = 0.0f;
    bool loggedCharacter_ = false;
};

#endif
