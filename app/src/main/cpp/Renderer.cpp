#include "Renderer.h"

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <game-activity/GameActivity.h>

#include <GLES3/gl3.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "AndroidOut.h"
#include "core/AssetFile.h"
#include "graphics/GltfLoader.h"
#include "ui/MainMenuScene.h"
#include "ui/MenuAudio.h"
#include "ui/SceneManager.h"
#include "ui/SettingsStore.h"
#include "ui/UiCanvas.h"

namespace {

constexpr const char *kModelAsset = "models/vera_costa_block.glb";

bool readShader(
        AAssetManager *manager,
        const char *path,
        std::string &source,
        std::string &error) {
    if (!AssetFile::readText(manager, path, source, error)) {
        return false;
    }
    if (source.empty()) {
        error = std::string("shader is empty: ") + path;
        return false;
    }
    return true;
}

}  // namespace

Renderer::Renderer(android_app *app)
        : app_(app),
          display_(EGL_NO_DISPLAY),
          surface_(EGL_NO_SURFACE),
          context_(EGL_NO_CONTEXT),
          width_(0),
          height_(0) {
    initRenderer();
}

Renderer::~Renderer() {
    if (audio_) {
        audio_->stop();
    }
    if (menu_) {
        menu_->destroy();
    }
    if (canvas_) {
        canvas_->destroy();
    }
    enemies_.clear();
    meshes_.clear();
    sceneShader_.reset();
    skyPass_.reset();
    menu_.reset();
    canvas_.reset();
    audio_.reset();
    scenes_.reset();
    if (display_ != EGL_NO_DISPLAY) {
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (context_ != EGL_NO_CONTEXT) {
            eglDestroyContext(display_, context_);
            context_ = EGL_NO_CONTEXT;
        }
        if (surface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, surface_);
            surface_ = EGL_NO_SURFACE;
        }
        eglTerminate(display_);
        display_ = EGL_NO_DISPLAY;
    }
}

void Renderer::initRenderer() {
    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLint configCount = 0;
    eglChooseConfig(display, attribs, nullptr, 0, &configCount);
    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[configCount]);
    eglChooseConfig(display, attribs, supportedConfigs.get(), configCount, &configCount);

    auto config = *std::find_if(
            supportedConfigs.get(),
            supportedConfigs.get() + configCount,
            [&display](const EGLConfig &candidate) {
                EGLint red = 0;
                EGLint green = 0;
                EGLint blue = 0;
                EGLint depth = 0;
                if (eglGetConfigAttrib(display, candidate, EGL_RED_SIZE, &red)
                    && eglGetConfigAttrib(display, candidate, EGL_GREEN_SIZE, &green)
                    && eglGetConfigAttrib(display, candidate, EGL_BLUE_SIZE, &blue)
                    && eglGetConfigAttrib(display, candidate, EGL_DEPTH_SIZE, &depth)) {
                    return red == 8 && green == 8 && blue == 8 && depth == 24;
                }
                return false;
            });

    EGLint format = 0;
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    EGLSurface surface = eglCreateWindowSurface(display, config, app_->window, nullptr);
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    EGLContext context = eglCreateContext(display, config, nullptr, contextAttribs);
    if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) {
        aout << "[NG] eglMakeCurrent failed" << std::endl;
        return;
    }

    display_ = display;
    surface_ = surface;
    context_ = context;
    width_ = -1;
    height_ = -1;

    aout << "[NG] GL_VENDOR " << glGetString(GL_VENDOR) << std::endl;
    aout << "[NG] GL_RENDERER " << glGetString(GL_RENDERER) << std::endl;
    aout << "[NG] GL_VERSION " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glDisable(GL_BLEND);
    glClearColor(0.30f, 0.50f, 0.78f, 1.0f);

    sceneReady_ = loadScene();
    aout << "[NG] world " << (sceneReady_ ? "ready" : "failed") << " " << sceneStatus_ << std::endl;

    loadSettings();
    applyRuntimeSettings();
    canvas_ = std::make_unique<UiCanvas>();
    menu_ = std::make_unique<MainMenuScene>();
    scenes_ = std::make_unique<SceneManager>();
    audio_ = std::make_unique<MenuAudio>();
    std::string uiVertex;
    std::string uiFragment;
    std::string uiError;
    if (!readShader(assets(), "shaders/ui.vert", uiVertex, uiError) ||
        !readShader(assets(), "shaders/ui.frag", uiFragment, uiError) ||
        !canvas_->create(uiVertex, uiFragment, uiError) ||
        !menu_->create(*this, uiError)) {
        aout << "[NG] menu init failed: " << uiError << std::endl;
    } else {
        aout << "[NG] menu ready background=" << (menu_->backgroundLoaded() ? 1 : 0) << std::endl;
    }
    std::string audioError;
    if (!audio_->start(assets(), "audio/menu_theme.wav", audioError)) {
        aout << "[NG] menu_audio unavailable: " << audioError << std::endl;
    }
    aout << "[NG] boot_scene=menu gameplay_starts_after_choice=1" << std::endl;
}

bool Renderer::loadScene() {
    AAssetManager *assets = app_->activity->assetManager;
    std::vector<uint8_t> modelBytes;
    std::string error;
    if (!AssetFile::readBinary(assets, kModelAsset, modelBytes, error)) {
        sceneStatus_ = error;
        return false;
    }

    CpuModel model;
    if (!GltfLoader::loadFromMemory(modelBytes.data(), modelBytes.size(), model, error)) {
        sceneStatus_ = error;
        return false;
    }

    std::string sceneVertex;
    std::string sceneFragment;
    std::string skyVertex;
    std::string skyFragment;
    if (!readShader(assets, "shaders/scene.vert", sceneVertex, error) ||
        !readShader(assets, "shaders/scene.frag", sceneFragment, error) ||
        !readShader(assets, "shaders/sky.vert", skyVertex, error) ||
        !readShader(assets, "shaders/sky.frag", skyFragment, error)) {
        sceneStatus_ = error;
        return false;
    }

    auto sceneShader = std::make_unique<SceneShader>();
    if (!sceneShader->create(sceneVertex, sceneFragment, error)) {
        sceneStatus_ = error;
        return false;
    }
    auto skyPass = std::make_unique<SkyPass>();
    if (!skyPass->create(skyVertex, skyFragment, error)) {
        sceneStatus_ = error;
        return false;
    }

    std::vector<StaticMesh> meshes;
    meshes.reserve(model.primitives.size());
    size_t triangles = 0;
    for (const CpuPrimitive &primitive : model.primitives) {
        StaticMesh mesh;
        if (!mesh.upload(primitive)) {
            sceneStatus_ = "failed to upload " + primitive.name;
            return false;
        }
        triangles += primitive.indices.size() / 3;
        meshes.push_back(std::move(mesh));
    }

    sceneShader_ = std::move(sceneShader);
    skyPass_ = std::move(skyPass);
    meshes_ = std::move(meshes);
    std::string enemyError;
    if (!enemies_.load(assets, enemyError)) {
        aout << "[NG] enemies unavailable: " << enemyError << std::endl;
    } else {
        aout << "[NG] enemies=" << enemies_.count() << " model=models/enemy_guard.glb" << std::endl;
    }
    sceneStatus_ = "primitives=" + std::to_string(meshes_.size()) +
                   " triangles=" + std::to_string(triangles);
    return !meshes_.empty();
}

void Renderer::updateRenderArea() {
    EGLint width = 0;
    EGLint height = 0;
    eglQuerySurface(display_, surface_, EGL_WIDTH, &width);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height);
    if (width != width_ || height != height_) {
        width_ = width;
        height_ = height;
        glViewport(0, 0, width, height);
    }
}

void Renderer::update(float deltaSeconds) {
    if (scenes_ && scenes_->active() == SceneManager::SceneId::Gameplay) {
        camera_.update(deltaSeconds);
        enemies_.update(deltaSeconds, camera_.target());
        musicFade_ = std::max(0.0f, musicFade_ - deltaSeconds / 0.45f);
    } else {
        musicFade_ = std::min(1.0f, musicFade_ + deltaSeconds / 1.1f);
        if (menu_) {
            menu_->update(*this, deltaSeconds);
        }
    }
    if (scenes_) {
        const SceneManager::SceneId before = scenes_->active();
        scenes_->update(deltaSeconds);
        if (scenes_->active() != before) {
            aout << "[NG] scene_enter "
                 << (scenes_->active() == SceneManager::SceneId::Gameplay ? "gameplay" : "menu")
                 << std::endl;
        }
    }
    applyRuntimeSettings();
    if (audio_) {
        audio_->setGain(settings_.volume * musicFade_);
    }
    fpsAccum_ += deltaSeconds;
    frameCount_ += 1;
    if (fpsAccum_ >= 2.0f) {
        const float fps = static_cast<float>(frameCount_) / fpsAccum_;
        aout << "[NG] measured_fps=" << fps
             << " meshes=" << meshes_.size()
             << " scene=" << (scenes_ && scenes_->active() == SceneManager::SceneId::Gameplay ? "gameplay" : "menu")
             << " world=" << sceneStatus_ << std::endl;
        fpsAccum_ = 0.0f;
        frameCount_ = 0;
    }
}

void Renderer::renderWorld() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    const float scale = qualityViewportScale();
    const int viewWidth = std::max(1, static_cast<int>(static_cast<float>(width_) * scale));
    const int viewHeight = std::max(1, static_cast<int>(static_cast<float>(height_) * scale));
    const int viewX = (width_ - viewWidth) / 2;
    const int viewY = (height_ - viewHeight) / 2;
    glViewport(viewX, viewY, viewWidth, viewHeight);
    if (sceneReady_ && sceneShader_ && skyPass_) {
        skyPass_->draw();
        const float aspect = static_cast<float>(viewWidth) / static_cast<float>(viewHeight);
        const glm::mat4 view = camera_.view();
        const glm::mat4 projection = camera_.projection(aspect);
        const glm::vec3 eye = camera_.position();
        for (const StaticMesh &mesh : meshes_) {
            sceneShader_->draw(mesh, view, projection, eye);
        }
        enemies_.draw(*sceneShader_, view, projection, eye);
    }
    glViewport(0, 0, width_, height_);
    if (!sceneReady_ && canvas_ && menu_) {
        canvas_->begin(width_, height_);
        menu_->font().draw(
                *canvas_,
                "CENA 3D INDISPONIVEL",
                48.0f,
                static_cast<float>(height_) * 0.5f,
                0.7f,
                1.0f,
                0.85f,
                0.4f,
                1.0f);
        canvas_->end();
    }
}

void Renderer::render() {
    if (display_ == EGL_NO_DISPLAY) {
        return;
    }

    updateRenderArea();
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const bool gameplay = scenes_ && scenes_->active() == SceneManager::SceneId::Gameplay;
    if (gameplay) {
        renderWorld();
    } else if (menu_ && canvas_) {
        menu_->render(*this, *canvas_);
    }
    if (scenes_ && canvas_ && scenes_->fade() > 0.0f) {
        canvas_->begin(width_, height_);
        canvas_->drawSolid(
                0.0f,
                0.0f,
                static_cast<float>(width_),
                static_cast<float>(height_),
                0.0f,
                0.0f,
                0.0f,
                scenes_->fade());
        canvas_->end();
    }

    eglSwapBuffers(display_, surface_);
}

void Renderer::updateCameraFromPointers(uint32_t pointerCount, const GameActivityPointerAxes *pointers) {
    if (pointerCount >= 2 && pointers != nullptr) {
        const float x0 = GameActivityPointerAxes_getX(&pointers[0]);
        const float y0 = GameActivityPointerAxes_getY(&pointers[0]);
        const float x1 = GameActivityPointerAxes_getX(&pointers[1]);
        const float y1 = GameActivityPointerAxes_getY(&pointers[1]);
        const float distance = std::hypot(x0 - x1, y0 - y1);
        if (pinchPrimed_ && pinchDistance_ > 1.0f && distance > 1.0f) {
            camera_.multiplyDistance(pinchDistance_ / distance);
        }
        pinchDistance_ = distance;
        pinchPrimed_ = true;
        orbitPrimed_ = false;
        return;
    }

    pinchPrimed_ = false;
    if (pointerCount == 1 && pointers != nullptr) {
        const auto id = static_cast<int32_t>(pointers[0].id);
        const float x = GameActivityPointerAxes_getX(&pointers[0]);
        const float y = GameActivityPointerAxes_getY(&pointers[0]);
        const auto existing = pointers_.find(id);
        if (orbitPrimed_ && existing != pointers_.end()) {
            camera_.addOrbit(x - existing->second.x, y - existing->second.y);
        }
        pointers_[id] = PointerSample{x, y};
        orbitPrimed_ = true;
    }
}

void Renderer::handleInput() {
    android_input_buffer *inputBuffer = android_app_swap_input_buffers(app_);
    if (inputBuffer == nullptr) {
        return;
    }

    const bool menuActive = !scenes_ || scenes_->active() == SceneManager::SceneId::MainMenu;
    const bool allowInput = !scenes_ || scenes_->acceptsInput();
    for (uint64_t eventIndex = 0; eventIndex < inputBuffer->motionEventsCount; ++eventIndex) {
        GameActivityMotionEvent &event = inputBuffer->motionEvents[eventIndex];
        const int action = event.action & AMOTION_EVENT_ACTION_MASK;
        const uint32_t pointerIndex = (static_cast<uint32_t>(event.action) &
                                        AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                                       >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        if (!allowInput) {
            continue;
        }
        if (menuActive) {
            if (action == AMOTION_EVENT_ACTION_MOVE) {
                continue;
            }
            if (pointerIndex >= event.pointerCount) {
                continue;
            }
            const GameActivityPointerAxes &pointer = event.pointers[pointerIndex];
            routeMenuPointer(
                    action,
                    GameActivityPointerAxes_getX(&pointer),
                    GameActivityPointerAxes_getY(&pointer));
            continue;
        }

        if (action == AMOTION_EVENT_ACTION_MOVE) {
            updateCameraFromPointers(event.pointerCount, event.pointers);
            continue;
        }

        if (pointerIndex >= event.pointerCount) {
            continue;
        }
        GameActivityPointerAxes &pointer = event.pointers[pointerIndex];
        const auto id = static_cast<int32_t>(pointer.id);
        const float x = GameActivityPointerAxes_getX(&pointer);
        const float y = GameActivityPointerAxes_getY(&pointer);
        if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
            pointers_[id] = PointerSample{x, y};
            orbitPrimed_ = event.pointerCount == 1;
            pinchPrimed_ = false;
        } else if (action == AMOTION_EVENT_ACTION_UP ||
                   action == AMOTION_EVENT_ACTION_POINTER_UP ||
                   action == AMOTION_EVENT_ACTION_CANCEL) {
            pointers_.erase(id);
            orbitPrimed_ = false;
            pinchPrimed_ = false;
        }
    }
    android_app_clear_motion_events(inputBuffer);
    android_app_clear_key_events(inputBuffer);
}

int Renderer::screenWidth() const {
    return width_;
}

int Renderer::screenHeight() const {
    return height_;
}

AAssetManager *Renderer::assets() const {
    return app_->activity->assetManager;
}

const GameSettings &Renderer::settings() const {
    return settings_;
}

void Renderer::applySettings(const GameSettings &settings) {
    settings_ = settings;
    if (settings_.language < 0 || settings_.language > 2) {
        settings_.language = 0;
    }
    if (settings_.quality < 0 || settings_.quality > 2) {
        settings_.quality = 1;
    }
    settings_.volume = std::clamp(settings_.volume, 0.0f, 1.0f);
    settings_.lookSensitivity = std::clamp(settings_.lookSensitivity, 0.4f, 2.0f);
    applyRuntimeSettings();
    if (!saveSettings()) {
        aout << "[NG] settings save failed " << internalPath("settings.cfg") << std::endl;
    } else {
        aout << "[NG] settings language=" << settings_.language
             << " quality=" << settings_.quality
             << " volume=" << settings_.volume
             << " sensitivity=" << settings_.lookSensitivity
             << " invert=" << (settings_.invertLook ? 1 : 0) << std::endl;
    }
}

bool Renderer::hasCampaignSave() const {
    return SettingsStore::fileHasAtLeast(internalPath("campaign.sav"), 8);
}

void Renderer::startNewGame() {
    if (!loggedCharacter_) {
        loggedCharacter_ = true;
        AAssetDir *directory = AAssetManager_openDir(assets(), "characters");
        const char *file = directory == nullptr ? nullptr : AAssetDir_getNextFileName(directory);
        if (file == nullptr) {
            aout << "[NG] no player character in assets/characters; new game enters Vera Costa with "
                 << enemies_.count() << " guard enemies" << std::endl;
        } else {
            aout << "[NG] character file found " << file << " but the player rig is not loaded in this build" << std::endl;
        }
        if (directory != nullptr) {
            AAssetDir_close(directory);
        }
    }
    if (scenes_) {
        scenes_->request(SceneManager::SceneId::Gameplay);
    }
}

void Renderer::continueGame() {
    if (!hasCampaignSave()) {
        aout << "[NG] continue ignored, no campaign.sav" << std::endl;
        return;
    }
    aout << "[NG] continue found " << internalPath("campaign.sav")
         << " ; mission restore is not implemented, entering the 3D block" << std::endl;
    if (scenes_) {
        scenes_->request(SceneManager::SceneId::Gameplay);
    }
}

void Renderer::quitGame() {
    GameActivity_finish(app_->activity);
}

void Renderer::routeMenuPointer(int action, float x, float y) {
    if (!menu_) {
        return;
    }
    if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        menu_->onPointerDown(*this, x, y);
    } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP) {
        menu_->onPointerUp(*this, x, y);
    } else if (action == AMOTION_EVENT_ACTION_CANCEL) {
        menu_->onPointerCancel();
    }
}

void Renderer::loadSettings() {
    if (!SettingsStore::load(internalPath("settings.cfg"), settings_)) {
        aout << "[NG] settings default" << std::endl;
    }
}

bool Renderer::saveSettings() const {
    return SettingsStore::save(internalPath("settings.cfg"), settings_);
}

void Renderer::applyRuntimeSettings() {
    camera_.setLookSensitivity(settings_.lookSensitivity);
    camera_.setInvertLook(settings_.invertLook);
    camera_.setFarPlane(qualityFarPlane());
}

std::string Renderer::internalPath(const char *name) const {
    const char *base = app_ != nullptr && app_->activity != nullptr ? app_->activity->internalDataPath : nullptr;
    if (base == nullptr || base[0] == '\0') {
        return std::string("/data/local/tmp/") + name;
    }
    return std::string(base) + "/" + name;
}

float Renderer::qualityFarPlane() const {
    if (settings_.quality <= 0) {
        return 90.0f;
    }
    if (settings_.quality == 1) {
        return 180.0f;
    }
    return 300.0f;
}

float Renderer::qualityViewportScale() const {
    if (settings_.quality <= 0) {
        return 0.70f;
    }
    if (settings_.quality == 1) {
        return 0.85f;
    }
    return 1.0f;
}
