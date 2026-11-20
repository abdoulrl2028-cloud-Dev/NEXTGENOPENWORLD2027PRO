#include "UIButton.h"

#include "BitmapFont.h"
#include "UiCanvas.h"

bool UIButton::contains(float px, float py) const {
    return px >= x && py >= y && px <= x + w && py <= y + h;
}

void UIButton::draw(UiCanvas &canvas, const BitmapFont &font) const {
    const float alpha = appear * (enabled ? 1.0f : 0.45f);
    float red = 0.07f;
    float green = 0.09f;
    float blue = 0.12f;
    if (!enabled) {
        red = 0.05f;
        green = 0.05f;
        blue = 0.06f;
    } else if (pressed) {
        red = 0.22f;
        green = 0.30f;
        blue = 0.38f;
    } else if (selected) {
        red = 0.16f;
        green = 0.30f;
        blue = 0.24f;
    }
    canvas.drawSolid(x, y, w, h, red, green, blue, 0.90f * alpha);
    canvas.drawSolid(x, y, w, 2.0f, 0.85f, 0.78f, 0.55f, 0.85f * alpha);

    const float scale = h * 0.46f / font.lineHeight();
    const float textWidth = font.measure(label, scale);
    const float textX = x + (w - textWidth) * 0.5f;
    const float baseline = y + h * 0.68f;
    const float textAlpha = enabled ? alpha : alpha * 0.75f;
    font.draw(canvas, label, textX, baseline, scale, 0.95f, 0.95f, 0.93f, textAlpha);
}
