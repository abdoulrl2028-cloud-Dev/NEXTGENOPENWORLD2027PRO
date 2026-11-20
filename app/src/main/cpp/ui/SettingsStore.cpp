#include "SettingsStore.h"

#include <fstream>

bool SettingsStore::load(const std::string &path, GameSettings &settings) {
    std::ifstream input(path);
    if (!input) {
        return false;
    }
    std::string key;
    while (input >> key) {
        if (key == "language") {
            input >> settings.language;
        } else if (key == "quality") {
            input >> settings.quality;
        } else if (key == "volume") {
            input >> settings.volume;
        } else if (key == "sensitivity") {
            input >> settings.lookSensitivity;
        } else if (key == "invert") {
            int invert = 0;
            input >> invert;
            settings.invertLook = invert != 0;
        }
    }
    if (settings.language < 0 || settings.language > 2) {
        settings.language = 0;
    }
    if (settings.quality < 0 || settings.quality > 2) {
        settings.quality = 1;
    }
    if (!(settings.volume >= 0.0f && settings.volume <= 1.0f)) {
        settings.volume = 0.7f;
    }
    if (!(settings.lookSensitivity >= 0.4f && settings.lookSensitivity <= 2.0f)) {
        settings.lookSensitivity = 1.0f;
    }
    return true;
}

bool SettingsStore::save(const std::string &path, const GameSettings &settings) {
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        return false;
    }
    output << "language " << settings.language << "\n";
    output << "quality " << settings.quality << "\n";
    output << "volume " << settings.volume << "\n";
    output << "sensitivity " << settings.lookSensitivity << "\n";
    output << "invert " << (settings.invertLook ? 1 : 0) << "\n";
    return output.good();
}

bool SettingsStore::fileHasAtLeast(const std::string &path, size_t minimumBytes) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        return false;
    }
    const std::streamoff size = input.tellg();
    return size >= static_cast<std::streamoff>(minimumBytes);
}
