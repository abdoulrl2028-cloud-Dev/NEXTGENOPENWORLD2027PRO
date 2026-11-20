#include "MainMenuScene.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

#include "AndroidOut.h"
#include "MenuContext.h"
#include "MenuLayout.h"
#include "UiCanvas.h"

namespace {

const char *textTitle() {
    return "NEXT GEN OPEN WORLD";
}

const char *textYear() {
    return "2027 PRO";
}

}  // namespace

bool MainMenuScene::create(MenuContext &context, std::string &error) {
    destroy();
    if (!font_.load(context.assets(), "ui/ui_font.png", "ui/ui_font.metrics", error)) {
        return false;
    }
    background_ = TextureLoader::loadAsset(context.assets(), "images/menu_background.png", 2048);
    if (!background_.ok) {
        aout << "[NG] menu_background missing: " << background_.error << std::endl;
    }
    ready_ = true;
    lastWidth_ = -1;
    return true;
}

void MainMenuScene::destroy() {
    background_.destroy();
    font_.destroy();
    buttons_.clear();
    ready_ = false;
}

void MainMenuScene::rebuild(MenuContext &context) {
    std::vector<MenuAction> actions;
    if (page_ == Page::Root) {
        actions = {
                MenuAction::NewGame,
                MenuAction::ContinueGame,
                MenuAction::Settings,
                MenuAction::Language,
                MenuAction::Quit};
    } else if (page_ == Page::Settings) {
        actions = {
                MenuAction::QualityLow,
                MenuAction::QualityMedium,
                MenuAction::QualityHigh,
                MenuAction::VolumeDown,
                MenuAction::VolumeUp,
                MenuAction::SensitivityDown,
                MenuAction::SensitivityUp,
                MenuAction::InvertLook,
                MenuAction::Back};
    } else {
        actions = {
                MenuAction::LanguagePortuguese,
                MenuAction::LanguageEnglish,
                MenuAction::LanguageFrench,
                MenuAction::Back};
    }
    layoutMenuColumn(
            static_cast<float>(context.screenWidth()),
            static_cast<float>(context.screenHeight()),
            actions,
            buttons_,
            titleX_,
            titleY_,
            titleW_,
            panelX_,
            panelY_,
            panelW_,
            panelH_);
    titleH_ = buttons_.empty() ? 48.0f : std::max(24.0f, buttons_.front().y - titleY_);
    refreshLabels(context);
    lastWidth_ = context.screenWidth();
    lastHeight_ = context.screenHeight();
    for (const UIButton &button : buttons_) {
        aout << "[NG] button id=" << button.id
             << " x=" << button.x << " y=" << button.y
             << " w=" << button.w << " h=" << button.h
             << " enabled=" << (button.enabled ? 1 : 0) << std::endl;
    }
}

void MainMenuScene::refreshLabels(const MenuContext &context) {
    const bool hasSave = context.hasCampaignSave();
    const GameSettings &settings = context.settings();
    for (UIButton &button : buttons_) {
        button.label = labelFor(context, button.id);
        button.enabled = true;
        button.selected = false;
        if (button.id == static_cast<int>(MenuAction::ContinueGame)) {
            button.enabled = hasSave;
        } else if (button.id == static_cast<int>(MenuAction::QualityLow)) {
            button.selected = settings.quality == 0;
        } else if (button.id == static_cast<int>(MenuAction::QualityMedium)) {
            button.selected = settings.quality == 1;
        } else if (button.id == static_cast<int>(MenuAction::QualityHigh)) {
            button.selected = settings.quality == 2;
        } else if (button.id == static_cast<int>(MenuAction::LanguagePortuguese)) {
            button.selected = settings.language == 0;
        } else if (button.id == static_cast<int>(MenuAction::LanguageEnglish)) {
            button.selected = settings.language == 1;
        } else if (button.id == static_cast<int>(MenuAction::LanguageFrench)) {
            button.selected = settings.language == 2;
        } else if (button.id == static_cast<int>(MenuAction::InvertLook)) {
            button.selected = settings.invertLook;
        }
    }
}

std::string MainMenuScene::labelFor(const MenuContext &context, int action) const {
    const int language = context.settings().language;
    const auto pick = [language](const char *portuguese, const char *english, const char *french) {
        if (language == 1) {
            return english;
        }
        if (language == 2) {
            return french;
        }
        return portuguese;
    };
    char numeric[64];
    const GameSettings &settings = context.settings();
    switch (static_cast<MenuAction>(action)) {
        case MenuAction::NewGame:
            return pick("NOVO JOGO", "NEW GAME", "NOUVELLE PARTIE");
        case MenuAction::ContinueGame:
            return pick("CONTINUAR", "CONTINUE", "CONTINUER");
        case MenuAction::Settings:
            return pick("CONFIGURAÇÕES", "SETTINGS", "PARAMÈTRES");
        case MenuAction::Language:
            return pick("IDIOMA", "LANGUAGE", "LANGUE");
        case MenuAction::Quit:
            return pick("SAIR", "EXIT", "QUITTER");
        case MenuAction::QualityLow:
            return pick("GRÁFICOS BAIXO", "GRAPHICS LOW", "GRAPHIQUES BAS");
        case MenuAction::QualityMedium:
            return pick("GRÁFICOS MÉDIO", "GRAPHICS MEDIUM", "GRAPHIQUES MOYEN");
        case MenuAction::QualityHigh:
            return pick("GRÁFICOS ALTO", "GRAPHICS HIGH", "GRAPHIQUES ÉLEVÉ");
        case MenuAction::VolumeDown:
            std::snprintf(numeric, sizeof(numeric), "%s %d%%",
                          pick("VOLUME -", "VOLUME -", "VOLUME -"),
                          static_cast<int>(settings.volume * 100.0f + 0.5f));
            return numeric;
        case MenuAction::VolumeUp:
            std::snprintf(numeric, sizeof(numeric), "%s %d%%",
                          pick("VOLUME +", "VOLUME +", "VOLUME +"),
                          static_cast<int>(settings.volume * 100.0f + 0.5f));
            return numeric;
        case MenuAction::SensitivityDown:
            std::snprintf(numeric, sizeof(numeric), "%s %.1f",
                          pick("OLHAR -", "LOOK -", "REGARD -"),
                          settings.lookSensitivity);
            return numeric;
        case MenuAction::SensitivityUp:
            std::snprintf(numeric, sizeof(numeric), "%s %.1f",
                          pick("OLHAR +", "LOOK +", "REGARD +"),
                          settings.lookSensitivity);
            return numeric;
        case MenuAction::InvertLook:
            if (settings.invertLook) {
                return pick("INVERTER OLHAR: SIM", "INVERT LOOK: ON", "INVERSER REGARD: OUI");
            }
            return pick("INVERTER OLHAR: NÃO", "INVERT LOOK: OFF", "INVERSER REGARD: NON");
        case MenuAction::LanguagePortuguese:
            return "PORTUGUÊS";
        case MenuAction::LanguageEnglish:
            return "ENGLISH";
        case MenuAction::LanguageFrench:
            return "FRANÇAIS";
        case MenuAction::Back:
            return pick("VOLTAR", "BACK", "RETOUR");
    }
    return "";
}

void MainMenuScene::activate(MenuContext &context, int action) {
    GameSettings settings = context.settings();
    switch (static_cast<MenuAction>(action)) {
        case MenuAction::NewGame:
            aout << "[NG] menu_new_game" << std::endl;
            context.startNewGame();
            break;
        case MenuAction::ContinueGame:
            if (context.hasCampaignSave()) {
                aout << "[NG] menu_continue" << std::endl;
                context.continueGame();
            }
            break;
        case MenuAction::Settings:
            page_ = Page::Settings;
            introTime_ = 0.0f;
            lastWidth_ = -1;
            break;
        case MenuAction::Language:
            page_ = Page::Language;
            introTime_ = 0.0f;
            lastWidth_ = -1;
            break;
        case MenuAction::Quit:
            aout << "[NG] menu_quit" << std::endl;
            context.quitGame();
            break;
        case MenuAction::QualityLow:
            settings.quality = 0;
            context.applySettings(settings);
            break;
        case MenuAction::QualityMedium:
            settings.quality = 1;
            context.applySettings(settings);
            break;
        case MenuAction::QualityHigh:
            settings.quality = 2;
            context.applySettings(settings);
            break;
        case MenuAction::VolumeDown:
            settings.volume = std::max(0.0f, settings.volume - 0.1f);
            context.applySettings(settings);
            break;
        case MenuAction::VolumeUp:
            settings.volume = std::min(1.0f, settings.volume + 0.1f);
            context.applySettings(settings);
            break;
        case MenuAction::SensitivityDown:
            settings.lookSensitivity = std::max(0.4f, settings.lookSensitivity - 0.1f);
            context.applySettings(settings);
            break;
        case MenuAction::SensitivityUp:
            settings.lookSensitivity = std::min(2.0f, settings.lookSensitivity + 0.1f);
            context.applySettings(settings);
            break;
        case MenuAction::InvertLook:
            settings.invertLook = !settings.invertLook;
            context.applySettings(settings);
            break;
        case MenuAction::LanguagePortuguese:
            settings.language = 0;
            context.applySettings(settings);
            break;
        case MenuAction::LanguageEnglish:
            settings.language = 1;
            context.applySettings(settings);
            break;
        case MenuAction::LanguageFrench:
            settings.language = 2;
            context.applySettings(settings);
            break;
        case MenuAction::Back:
            page_ = Page::Root;
            introTime_ = 0.0f;
            lastWidth_ = -1;
            break;
    }
}

void MainMenuScene::update(MenuContext &context, float deltaSeconds) {
    if (!ready_) {
        return;
    }
    if (context.screenWidth() != lastWidth_ || context.screenHeight() != lastHeight_) {
        rebuild(context);
    } else {
        refreshLabels(context);
    }
    time_ += deltaSeconds;
    introTime_ += deltaSeconds;
    for (size_t index = 0; index < buttons_.size(); ++index) {
        const float delay = 0.28f + static_cast<float>(index) * 0.07f;
        buttons_[index].appear = std::clamp((introTime_ - delay) / 0.32f, 0.0f, 1.0f);
    }
}

void MainMenuScene::render(MenuContext &context, UiCanvas &canvas) {
    if (!ready_ || context.screenWidth() <= 0 || context.screenHeight() <= 0) {
        return;
    }
    const float screenWidth = static_cast<float>(context.screenWidth());
    const float screenHeight = static_cast<float>(context.screenHeight());
    canvas.begin(context.screenWidth(), context.screenHeight());
    canvas.drawSolid(0.0f, 0.0f, screenWidth, screenHeight * 0.55f, 0.05f, 0.08f, 0.12f, 1.0f);
    canvas.drawSolid(0.0f, screenHeight * 0.55f, screenWidth, screenHeight * 0.45f, 0.10f, 0.12f, 0.14f, 1.0f);

    if (background_.ok) {
        FittedImage fitted = fitContain(screenWidth, screenHeight, static_cast<float>(background_.width), static_cast<float>(background_.height));
        const float slackX = std::max(0.0f, (screenWidth - fitted.w) * 0.5f);
        const float slackY = std::max(0.0f, (screenHeight - fitted.h) * 0.5f);
        const float shiftX = std::sin(time_ * 0.22f) * std::min(slackX, 12.0f);
        const float shiftY = std::cos(time_ * 0.16f) * std::min(slackY, 8.0f);
        canvas.drawTextured(
                fitted.x + shiftX,
                fitted.y + shiftY,
                fitted.w,
                fitted.h,
                background_.id,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f,
                1.0f);
    }

    canvas.drawSolid(panelX_, panelY_, panelW_, panelH_, 0.02f, 0.03f, 0.04f, 0.62f);
    const float titleAlpha = std::clamp(introTime_ / 0.7f, 0.0f, 1.0f);
    const float titleScale = std::min(titleW_ / std::max(font_.measure(textTitle(), 1.0f), 1.0f), titleH_ * 0.34f / font_.lineHeight());
    const float titleOffset = (1.0f - titleAlpha) * 28.0f;
    font_.draw(canvas, textTitle(), titleX_, titleY_ + titleH_ * 0.42f + titleOffset, titleScale, 0.96f, 0.93f, 0.84f, titleAlpha);
    font_.draw(canvas, textYear(), titleX_, titleY_ + titleH_ * 0.84f + titleOffset, titleScale * 0.72f, 0.96f, 0.93f, 0.84f, titleAlpha);

    if (!background_.ok) {
        const char *missing = context.settings().language == 1
                                      ? "ADD images/menu_background.png"
                                      : (context.settings().language == 2
                                                 ? "AJOUTER images/menu_background.png"
                                                 : "ADICIONE images/menu_background.png");
        font_.draw(
                canvas,
                missing,
                panelX_ + panelW_ + 28.0f,
                screenHeight * 0.55f,
                titleScale * 0.42f,
                0.95f,
                0.72f,
                0.45f,
                titleAlpha);
    }

    for (const UIButton &button : buttons_) {
        button.draw(canvas, font_);
    }
    canvas.end();
}

void MainMenuScene::onPointerDown(MenuContext &, float x, float y) {
    for (UIButton &button : buttons_) {
        button.pressed = button.enabled && button.appear > 0.92f && button.contains(x, y);
    }
}

void MainMenuScene::onPointerUp(MenuContext &context, float x, float y) {
    int fired = 0;
    for (UIButton &button : buttons_) {
        if (button.pressed && button.enabled && button.contains(x, y)) {
            fired = button.id;
        }
        button.pressed = false;
    }
    if (fired != 0) {
        activate(context, fired);
    }
}

void MainMenuScene::onPointerCancel() {
    for (UIButton &button : buttons_) {
        button.pressed = false;
    }
}
