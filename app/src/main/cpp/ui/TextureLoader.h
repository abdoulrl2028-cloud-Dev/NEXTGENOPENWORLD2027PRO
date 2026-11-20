#ifndef NEXTGEN_UI_TEXTURELOADER_H
#define NEXTGEN_UI_TEXTURELOADER_H

#include <android/asset_manager.h>
#include <GLES3/gl3.h>

#include <string>

struct LoadedTexture {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    bool ok = false;
    std::string error;
    size_t decodedBytes = 0;

    void destroy();
};

class TextureLoader {
public:
    static LoadedTexture loadAsset(AAssetManager *manager, const char *path, int maxEdge = 2048);
};

#endif
