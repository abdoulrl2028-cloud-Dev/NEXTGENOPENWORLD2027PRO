#ifndef NEXTGEN_UI_MENUCONTEXT_H
#define NEXTGEN_UI_MENUCONTEXT_H

#include <android/asset_manager.h>

#include "GameSettings.h"

class MenuContext {
public:
    virtual ~MenuContext() = default;

    virtual int screenWidth() const = 0;
    virtual int screenHeight() const = 0;
    virtual AAssetManager *assets() const = 0;
    virtual const GameSettings &settings() const = 0;
    virtual void applySettings(const GameSettings &settings) = 0;
    virtual bool hasCampaignSave() const = 0;
    virtual void startNewGame() = 0;
    virtual void continueGame() = 0;
    virtual void quitGame() = 0;
};

#endif
