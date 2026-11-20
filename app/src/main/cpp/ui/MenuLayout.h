#ifndef NEXTGEN_UI_MENULAYOUT_H
#define NEXTGEN_UI_MENULAYOUT_H

#include <algorithm>
#include <vector>

#include "UIButton.h"

enum class MenuAction {
    NewGame = 1,
    ContinueGame,
    Settings,
    Language,
    Quit,
    QualityLow,
    QualityMedium,
    QualityHigh,
    VolumeDown,
    VolumeUp,
    SensitivityDown,
    SensitivityUp,
    InvertLook,
    LanguagePortuguese,
    LanguageEnglish,
    LanguageFrench,
    Back
};

struct FittedImage {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

inline FittedImage fitContain(float screenWidth, float screenHeight, float imageWidth, float imageHeight) {
    FittedImage fitted;
    if (screenWidth <= 0.0f || screenHeight <= 0.0f || imageWidth <= 0.0f || imageHeight <= 0.0f) {
        return fitted;
    }
    const float scale = std::min(screenWidth / imageWidth, screenHeight / imageHeight);
    fitted.w = imageWidth * scale;
    fitted.h = imageHeight * scale;
    fitted.x = (screenWidth - fitted.w) * 0.5f;
    fitted.y = (screenHeight - fitted.h) * 0.5f;
    return fitted;
}

inline void layoutMenuColumn(
        float screenWidth,
        float screenHeight,
        const std::vector<MenuAction> &actions,
        std::vector<UIButton> &buttons,
        float &titleX,
        float &titleY,
        float &titleW,
        float &panelX,
        float &panelY,
        float &panelW,
        float &panelH) {
    buttons.clear();
    const int count = std::max(1, static_cast<int>(actions.size()));
    const float margin = screenHeight < 520.0f ? 8.0f : 16.0f;
    const float pad = 14.0f;
    const float usableHeight = std::max(160.0f, screenHeight - margin * 2.0f);
    float titleHeight = std::clamp(usableHeight * 0.13f, 42.0f, 92.0f);
    float gap = std::clamp(usableHeight * 0.012f, 6.0f, 14.0f);
    float buttonHeight =
            (usableHeight - titleHeight - gap - pad * 2.0f - gap * static_cast<float>(count - 1)) /
            static_cast<float>(count);
    if (buttonHeight > 66.0f) {
        buttonHeight = 66.0f;
    }
    if (buttonHeight < 36.0f) {
        titleHeight = 36.0f;
        gap = 4.0f;
        buttonHeight =
                (usableHeight - titleHeight - gap - pad * 2.0f - gap * static_cast<float>(count - 1)) /
                static_cast<float>(count);
    }
    float buttonWidth = std::min(screenWidth * 0.5f, 520.0f);
    buttonWidth = std::min(buttonWidth, screenWidth - margin * 2.0f - pad * 2.0f);
    panelW = buttonWidth + pad * 2.0f;
    panelH = titleHeight + gap + buttonHeight * static_cast<float>(count) +
             gap * static_cast<float>(count - 1) + pad * 2.0f;
    if (panelH > usableHeight) {
        const float fit = usableHeight / panelH;
        titleHeight *= fit;
        gap *= fit;
        buttonHeight *= fit;
        panelH = usableHeight;
    }
    panelX = std::max(margin, screenWidth * 0.055f);
    if (panelX + panelW > screenWidth - margin) {
        panelX = std::max(margin, screenWidth - margin - panelW);
    }
    panelY = std::max(margin, (screenHeight - panelH) * 0.5f);
    titleX = panelX + pad;
    titleY = panelY + pad;
    titleW = buttonWidth;
    float cursor = titleY + titleHeight + gap;
    buttons.reserve(static_cast<size_t>(count));
    for (MenuAction action : actions) {
        UIButton button;
        button.id = static_cast<int>(action);
        button.x = panelX + pad;
        button.y = cursor;
        button.w = buttonWidth;
        button.h = buttonHeight;
        buttons.push_back(button);
        cursor += buttonHeight + gap;
    }
}

#endif
