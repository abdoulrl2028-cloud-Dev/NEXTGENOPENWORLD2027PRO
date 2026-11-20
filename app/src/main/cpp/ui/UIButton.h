#ifndef NEXTGEN_UI_UIBUTTON_H
#define NEXTGEN_UI_UIBUTTON_H

#include <string>

class BitmapFont;
class UiCanvas;

class UIButton {
public:
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    std::string label;
    bool enabled = true;
    bool selected = false;
    bool pressed = false;
    float appear = 0.0f;

    bool contains(float px, float py) const;
    void draw(UiCanvas &canvas, const BitmapFont &font) const;
};

#endif
