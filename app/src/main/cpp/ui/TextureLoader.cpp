#include "TextureLoader.h"

#include <android/imagedecoder.h>

#include <algorithm>
#include <vector>

#include "AndroidOut.h"

void LoadedTexture::destroy() {
    if (id != 0) {
        glDeleteTextures(1, &id);
        id = 0;
    }
    width = 0;
    height = 0;
    ok = false;
    decodedBytes = 0;
}

LoadedTexture TextureLoader::loadAsset(AAssetManager *manager, const char *path, int maxEdge) {
    LoadedTexture texture;
    if (manager == nullptr || path == nullptr) {
        texture.error = "texture asset manager or path is null";
        return texture;
    }

    AAsset *asset = AAssetManager_open(manager, path, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        texture.error = std::string("texture asset not found: ") + path;
        return texture;
    }

    AImageDecoder *decoder = nullptr;
    const int created = AImageDecoder_createFromAAsset(asset, &decoder);
    if (created != ANDROID_IMAGE_DECODER_SUCCESS || decoder == nullptr) {
        AAsset_close(asset);
        texture.error = std::string("image decoder failed: ") + path;
        return texture;
    }

    AImageDecoder_setAndroidBitmapFormat(decoder, ANDROID_BITMAP_FORMAT_RGBA_8888);
    const AImageDecoderHeaderInfo *header = AImageDecoder_getHeaderInfo(decoder);
    int sourceWidth = AImageDecoderHeaderInfo_getWidth(header);
    int sourceHeight = AImageDecoderHeaderInfo_getHeight(header);
    if (sourceWidth <= 0 || sourceHeight <= 0) {
        AImageDecoder_delete(decoder);
        AAsset_close(asset);
        texture.error = std::string("image size is invalid: ") + path;
        return texture;
    }

    if (maxEdge > 0 && (sourceWidth > maxEdge || sourceHeight > maxEdge)) {
        const float scale = static_cast<float>(maxEdge) /
                            static_cast<float>(std::max(sourceWidth, sourceHeight));
        const int targetWidth = std::max(1, static_cast<int>(sourceWidth * scale));
        const int targetHeight = std::max(1, static_cast<int>(sourceHeight * scale));
        AImageDecoder_setTargetSize(decoder, targetWidth, targetHeight);
        header = AImageDecoder_getHeaderInfo(decoder);
    }

    const int width = AImageDecoderHeaderInfo_getWidth(header);
    const int height = AImageDecoderHeaderInfo_getHeight(header);
    const size_t stride = AImageDecoder_getMinimumStride(decoder);
    std::vector<uint8_t> pixels(stride * static_cast<size_t>(height));
    const int decoded = AImageDecoder_decodeImage(decoder, pixels.data(), stride, pixels.size());
    AImageDecoder_delete(decoder);
    AAsset_close(asset);
    if (decoded != ANDROID_IMAGE_DECODER_SUCCESS) {
        texture.error = std::string("image decode failed: ") + path;
        return texture;
    }

    glGenTextures(1, &texture.id);
    glBindTexture(GL_TEXTURE_2D, texture.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(stride / 4));
    glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            pixels.data());
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    texture.width = width;
    texture.height = height;
    texture.decodedBytes = pixels.size();
    texture.ok = texture.id != 0;
    if (!texture.ok) {
        texture.error = std::string("opengl texture failed: ") + path;
    }
    aout << "[NG] texture " << path << " " << width << "x" << height
         << " bytes=" << texture.decodedBytes << std::endl;
    return texture;
}
