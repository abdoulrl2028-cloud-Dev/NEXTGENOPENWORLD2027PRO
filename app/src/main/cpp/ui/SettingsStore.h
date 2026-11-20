#ifndef NEXTGEN_UI_SETTINGSSTORE_H
#define NEXTGEN_UI_SETTINGSSTORE_H

#include <string>

#include "GameSettings.h"

class SettingsStore {
public:
    static bool load(const std::string &path, GameSettings &settings);
    static bool save(const std::string &path, const GameSettings &settings);
    static bool fileHasAtLeast(const std::string &path, size_t minimumBytes);
};

#endif
