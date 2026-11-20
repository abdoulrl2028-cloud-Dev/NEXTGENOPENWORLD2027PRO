#ifndef NEXTGEN_UI_BITMAPFONT_H
#define NEXTGEN_UI_BITMAPFONT_H

#include <android/asset_manager.h>

#include <string>
#include <unordered_map>

#include "TextureLoader.h"

class UiCanvas;

struct Glyph {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float bearingX = 0.0f;
    float bearingY = 0.0f;
    float advance = 0.0f;
};

class BitmapFont {
public:
    bool load(AAssetManager *manager, const char *imagePath, const char *metricsPath, std::string &error);
    void destroy();
    float lineHeight() const { return lineHeight_; }
    float measure(const std::string &text, float scale) const;
    void draw(
            UiCanvas &canvas,
            const std::string &text,
            float x,
            float baseline,
            float scale,
            float red,
            float green,
            float blue,
            float alpha) const;

private:
    LoadedTexture texture_;
    std::unordered_map<uint32_t, Glyph> glyphs_;
    float lineHeight_ = 1.0f;
};

#endif
