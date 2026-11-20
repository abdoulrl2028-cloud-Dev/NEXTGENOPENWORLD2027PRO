#include "AssetFile.h"

bool AssetFile::readBinary(
        AAssetManager *manager,
        const char *path,
        std::vector<uint8_t> &out,
        std::string &error) {
    out.clear();
    if (manager == nullptr || path == nullptr) {
        error = "asset manager or path is null";
        return false;
    }

    AAsset *asset = AAssetManager_open(manager, path, AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        error = std::string("asset not found: ") + path;
        return false;
    }

    const off_t length = AAsset_getLength(asset);
    if (length < 0) {
        AAsset_close(asset);
        error = std::string("asset length is invalid: ") + path;
        return false;
    }

    out.resize(static_cast<size_t>(length));
    size_t copied = 0;
    while (copied < out.size()) {
        const int read = AAsset_read(
                asset,
                out.data() + copied,
                out.size() - copied);
        if (read <= 0) {
            AAsset_close(asset);
            error = std::string("asset read failed: ") + path;
            out.clear();
            return false;
        }
        copied += static_cast<size_t>(read);
    }
    AAsset_close(asset);
    return true;
}

bool AssetFile::readText(
        AAssetManager *manager,
        const char *path,
        std::string &out,
        std::string &error) {
    std::vector<uint8_t> bytes;
    if (!readBinary(manager, path, bytes, error)) {
        return false;
    }
    out.assign(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    return true;
}
