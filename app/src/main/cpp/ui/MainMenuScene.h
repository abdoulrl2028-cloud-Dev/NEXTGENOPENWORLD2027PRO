#ifndef NEXTGEN_UI_MAINMENUSCENE_H
#define NEXTGEN_UI_MAINMENUSCENE_H

#include <string>
#include <vector>

#include "BitmapFont.h"
#include "TextureLoader.h"
#include "UIButton.h"

class MenuContext;
class UiCanvas;

class MainMenuScene {
public:
    bool create(MenuContext &context, std::string &error);
    void destroy();
    void update(MenuContext &context, float deltaSeconds);
    void render(MenuContext &context, UiCanvas &canvas);
    void onPointerDown(MenuContext &context, float x, float y);
    void onPointerUp(MenuContext &context, float x, float y);
    void onPointerCancel();
    const BitmapFont &font() const { return font_; }
    bool backgroundLoaded() const { return background_.ok; }

private:
    enum class Page {
        Root,
        Settings,
        Language
    };

    void rebuild(MenuContext &context);
    void refreshLabels(const MenuContext &context);
    void activate(MenuContext &context, int action);
    std::string labelFor(const MenuContext &context, int action) const;

    LoadedTexture background_;
    BitmapFont font_;
    std::vector<UIButton> buttons_;
    Page page_ = Page::Root;
    float time_ = 0.0f;
    float introTime_ = 0.0f;
    int lastWidth_ = -1;
    int lastHeight_ = -1;
    float titleX_ = 0.0f;
    float titleY_ = 0.0f;
    float titleW_ = 0.0f;
    float titleH_ = 48.0f;
    float panelX_ = 0.0f;
    float panelY_ = 0.0f;
    float panelW_ = 0.0f;
    float panelH_ = 0.0f;
    bool ready_ = false;
};

#endif
