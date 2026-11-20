#ifndef NEXTGEN_CORE_ASSETFILE_H
#define NEXTGEN_CORE_ASSETFILE_H

#include <android/asset_manager.h>

#include <cstdint>
#include <string>
#include <vector>

class AssetFile {
public:
    static bool readBinary(
            AAssetManager *manager,
            const char *path,
            std::vector<uint8_t> &out,
            std::string &error);

    static bool readText(
            AAssetManager *manager,
            const char *path,
            std::string &out,
            std::string &error);
};

#endif
