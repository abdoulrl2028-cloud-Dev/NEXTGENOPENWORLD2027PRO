#include "BitmapFont.h"

#include <sstream>

#include "core/AssetFile.h"
#include "UiCanvas.h"

namespace {

bool nextCodepoint(const std::string &text, size_t &index, uint32_t &codepoint) {
    if (index >= text.size()) {
        return false;
    }
    const auto lead = static_cast<unsigned char>(text[index]);
    if (lead < 0x80) {
        codepoint = lead;
        index += 1;
        return true;
    }
    size_t extra = 0;
    if ((lead & 0xE0) == 0xC0) {
        codepoint = lead & 0x1F;
        extra = 1;
    } else if ((lead & 0xF0) == 0xE0) {
        codepoint = lead & 0x0F;
        extra = 2;
    } else if ((lead & 0xF8) == 0xF0) {
        codepoint = lead & 0x07;
        extra = 3;
    } else {
        codepoint = '?';
        index += 1;
        return true;
    }
    if (index + extra >= text.size()) {
        codepoint = '?';
        index = text.size();
        return true;
    }
    for (size_t step = 0; step < extra; ++step) {
        codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[index + 1 + step]) & 0x3F);
    }
    index += 1 + extra;
    return true;
}

}  // namespace

bool BitmapFont::load(
        AAssetManager *manager,
        const char *imagePath,
        const char *metricsPath,
        std::string &error) {
    destroy();
    texture_ = TextureLoader::loadAsset(manager, imagePath, 0);
    if (!texture_.ok) {
        error = texture_.error;
        return false;
    }
    std::string metrics;
    if (!AssetFile::readText(manager, metricsPath, metrics, error)) {
        destroy();
        return false;
    }

    std::istringstream stream(metrics);
    std::string header;
    int atlasWidth = 0;
    int atlasHeight = 0;
    stream >> header >> atlasWidth >> atlasHeight;
    stream >> header >> lineHeight_;
    if (header != "line" || atlasWidth <= 0 || atlasHeight <= 0 || lineHeight_ <= 0.0f) {
        error = "font metrics header is invalid";
        destroy();
        return false;
    }

    uint32_t codepoint = 0;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float bearingX = 0.0f;
    float bearingY = 0.0f;
    float advance = 0.0f;
    while (stream >> codepoint >> x >> y >> width >> height >> bearingX >> bearingY >> advance) {
        Glyph glyph;
        glyph.u0 = x / static_cast<float>(atlasWidth);
        glyph.v0 = y / static_cast<float>(atlasHeight);
        glyph.u1 = (x + width) / static_cast<float>(atlasWidth);
        glyph.v1 = (y + height) / static_cast<float>(atlasHeight);
        glyph.width = width;
        glyph.height = height;
        glyph.bearingX = bearingX;
        glyph.bearingY = bearingY;
        glyph.advance = advance;
        glyphs_[codepoint] = glyph;
    }
    if (glyphs_.empty()) {
        error = "font metrics contain no glyphs";
        destroy();
        return false;
    }
    return true;
}

void BitmapFont::destroy() {
    texture_.destroy();
    glyphs_.clear();
    lineHeight_ = 1.0f;
}

float BitmapFont::measure(const std::string &text, float scale) const {
    float width = 0.0f;
    size_t index = 0;
    uint32_t codepoint = 0;
    while (nextCodepoint(text, index, codepoint)) {
        const auto glyph = glyphs_.find(codepoint);
        if (glyph != glyphs_.end()) {
            width += glyph->second.advance * scale;
        }
    }
    return width;
}

void BitmapFont::draw(
        UiCanvas &canvas,
        const std::string &text,
        float x,
        float baseline,
        float scale,
        float red,
        float green,
        float blue,
        float alpha) const {
    if (!texture_.ok || alpha <= 0.0f) {
        return;
    }
    float pen = x;
    size_t index = 0;
    uint32_t codepoint = 0;
    while (nextCodepoint(text, index, codepoint)) {
        const auto found = glyphs_.find(codepoint);
        if (found == glyphs_.end()) {
            continue;
        }
        const Glyph &glyph = found->second;
        if (glyph.width > 0.0f && glyph.height > 0.0f) {
            canvas.drawTextured(
                    pen + glyph.bearingX * scale,
                    baseline - glyph.bearingY * scale,
                    glyph.width * scale,
                    glyph.height * scale,
                    texture_.id,
                    glyph.u0,
                    glyph.v0,
                    glyph.u1,
                    glyph.v1,
                    red,
                    green,
                    blue,
                    alpha);
        }
        pen += glyph.advance * scale;
    }
}
