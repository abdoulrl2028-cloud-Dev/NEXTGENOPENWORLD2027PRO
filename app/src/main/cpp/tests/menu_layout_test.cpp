#include "ui/MenuLayout.h"
#include "ui/SettingsStore.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool inside(const UIButton &button, float screenWidth, float screenHeight) {
    return button.w >= 36.0f && button.h >= 36.0f && button.x >= 0.0f && button.y >= 0.0f &&
           button.x + button.w <= screenWidth + 0.6f && button.y + button.h <= screenHeight + 0.6f;
}

bool noOverlap(const std::vector<UIButton> &buttons) {
    for (size_t i = 0; i < buttons.size(); ++i) {
        for (size_t j = i + 1; j < buttons.size(); ++j) {
            const bool separated = buttons[i].x + buttons[i].w <= buttons[j].x ||
                                   buttons[j].x + buttons[j].w <= buttons[i].x ||
                                   buttons[i].y + buttons[i].h <= buttons[j].y ||
                                   buttons[j].y + buttons[j].h <= buttons[i].y;
            if (!separated) {
                return false;
            }
        }
    }
    return true;
}

bool layoutFits(float screenWidth, float screenHeight, const std::vector<MenuAction> &actions) {
    std::vector<UIButton> buttons;
    float titleX = 0, titleY = 0, titleW = 0, panelX = 0, panelY = 0, panelW = 0, panelH = 0;
    layoutMenuColumn(screenWidth, screenHeight, actions, buttons, titleX, titleY, titleW, panelX, panelY, panelW, panelH);
    if (buttons.size() != actions.size() || panelX < 0.0f || panelY < 0.0f ||
        panelX + panelW > screenWidth + 0.6f || panelY + panelH > screenHeight + 0.6f) {
        std::cerr << "panel does not fit " << screenWidth << "x" << screenHeight << "\n";
        return false;
    }
    for (const UIButton &button : buttons) {
        if (!inside(button, screenWidth, screenHeight)) {
            std::cerr << "button outside " << button.x << "," << button.y << " " << button.w << "x" << button.h << "\n";
            return false;
        }
    }
    return noOverlap(buttons);
}

}  // namespace

int main() {
    const std::vector<MenuAction> root = {
            MenuAction::NewGame,
            MenuAction::ContinueGame,
            MenuAction::Settings,
            MenuAction::Language,
            MenuAction::Quit};
    const std::vector<MenuAction> settings = {
            MenuAction::QualityLow,
            MenuAction::QualityMedium,
            MenuAction::QualityHigh,
            MenuAction::VolumeDown,
            MenuAction::VolumeUp,
            MenuAction::SensitivityDown,
            MenuAction::SensitivityUp,
            MenuAction::InvertLook,
            MenuAction::Back};
    const float resolutions[][2] = {{1280, 720}, {1920, 1080}, {2400, 1080}, {720, 1280}, {800, 480}};
    for (const auto &resolution : resolutions) {
        if (!layoutFits(resolution[0], resolution[1], root) || !layoutFits(resolution[0], resolution[1], settings)) {
            return 1;
        }
    }

    const FittedImage wide = fitContain(1280, 720, 1920, 1080);
    const FittedImage tall = fitContain(720, 1280, 1920, 1080);
    const float wideAspect = wide.w / wide.h;
    const float tallAspect = tall.w / tall.h;
    if (std::fabs(wideAspect - (1920.0f / 1080.0f)) > 0.01f || std::fabs(tallAspect - wideAspect) > 0.01f) {
        std::cerr << "contain changed the image aspect\n";
        return 1;
    }
    if (wide.x < -0.1f || wide.y < -0.1f || wide.x + wide.w > 1280.1f || wide.y + wide.h > 720.1f ||
        tall.x < -0.1f || tall.y < -0.1f || tall.x + tall.w > 720.1f || tall.y + tall.h > 1280.1f) {
        std::cerr << "contain cropped or overflowed\n";
        return 1;
    }

    const std::string path = "/tmp/ng_settings_test.cfg";
    GameSettings stored;
    stored.language = 2;
    stored.quality = 0;
    stored.volume = 0.4f;
    stored.lookSensitivity = 1.5f;
    stored.invertLook = true;
    if (!SettingsStore::save(path, stored)) {
        std::cerr << "settings save failed\n";
        return 1;
    }
    GameSettings loaded;
    if (!SettingsStore::load(path, loaded) || loaded.language != 2 || loaded.quality != 0 ||
        std::fabs(loaded.volume - 0.4f) > 0.001f || std::fabs(loaded.lookSensitivity - 1.5f) > 0.001f ||
        !loaded.invertLook) {
        std::cerr << "settings load mismatch\n";
        return 1;
    }
    if (SettingsStore::fileHasAtLeast(path, 8) != true) {
        std::cerr << "existing settings file was not detected\n";
        return 1;
    }
    if (SettingsStore::fileHasAtLeast("/tmp/ng_missing_campaign.sav", 8)) {
        std::cerr << "missing save was reported as present\n";
        return 1;
    }
    std::cout << "menu_layout_test ok\n";
    return 0;
}
