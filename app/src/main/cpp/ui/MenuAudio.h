#ifndef NEXTGEN_UI_MENUAUDIO_H
#define NEXTGEN_UI_MENUAUDIO_H

#include <aaudio/AAudio.h>
#include <android/asset_manager.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

class MenuAudio {
public:
    bool start(AAssetManager *manager, const char *wavPath, std::string &error);
    void stop();
    void setGain(float gain);
    bool playing() const { return playing_; }
    int sampleRate() const { return sampleRate_; }
    void render(int16_t *output, int32_t frameCount);

private:
    std::vector<int16_t> samples_;
    std::atomic<int> cursor_{0};
    std::atomic<float> gain_{0.7f};
    AAudioStream *stream_ = nullptr;
    int sampleRate_ = 0;
    bool playing_ = false;
};

#endif
