#include "MenuAudio.h"

#include <aaudio/AAudio.h>

#include <cstring>

#include "AndroidOut.h"
#include "core/AssetFile.h"

namespace {

aaudio_data_callback_result_t menuAudioDataCallback(
        AAudioStream *,
        void *user,
        void *audioData,
        int32_t numFrames) {
    static_cast<MenuAudio *>(user)->render(static_cast<int16_t *>(audioData), numFrames);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

bool readWav(const std::vector<uint8_t> &bytes, int &sampleRate, std::vector<int16_t> &samples, std::string &error) {
    if (bytes.size() < 44 || std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0) {
        error = "menu audio is not a wav file";
        return false;
    }
    size_t offset = 12;
    int channels = 0;
    int bits = 0;
    const uint8_t *data = nullptr;
    size_t dataSize = 0;
    while (offset + 8 <= bytes.size()) {
        char id[5] = {};
        std::memcpy(id, bytes.data() + offset, 4);
        uint32_t size = 0;
        std::memcpy(&size, bytes.data() + offset + 4, 4);
        offset += 8;
        if (offset + size > bytes.size()) {
            break;
        }
        if (std::strcmp(id, "fmt ") == 0 && size >= 16) {
            uint16_t format = 0;
            uint16_t channelCount = 0;
            uint16_t bitDepth = 0;
            uint32_t rate = 0;
            std::memcpy(&format, bytes.data() + offset, 2);
            std::memcpy(&channelCount, bytes.data() + offset + 2, 2);
            std::memcpy(&rate, bytes.data() + offset + 4, 4);
            std::memcpy(&bitDepth, bytes.data() + offset + 14, 2);
            if (format != 1 || channelCount != 1 || bitDepth != 16) {
                error = "menu audio must be mono 16-bit pcm";
                return false;
            }
            channels = channelCount;
            bits = bitDepth;
            sampleRate = static_cast<int>(rate);
        } else if (std::strcmp(id, "data") == 0) {
            data = bytes.data() + offset;
            dataSize = size;
        }
        offset += size + (size & 1);
    }
    if (data == nullptr || channels != 1 || bits != 16 || sampleRate <= 0) {
        error = "menu audio wav is missing pcm data";
        return false;
    }
    samples.resize(dataSize / 2);
    std::memcpy(samples.data(), data, samples.size() * sizeof(int16_t));
    return !samples.empty();
}

}  // namespace

void MenuAudio::render(int16_t *output, int32_t frameCount) {
    if (output == nullptr || frameCount <= 0) {
        return;
    }
    if (samples_.empty()) {
        std::memset(output, 0, static_cast<size_t>(frameCount) * sizeof(int16_t));
        return;
    }
    const float gain = gain_.load();
    int cursor = cursor_.load();
    const int count = static_cast<int>(samples_.size());
    for (int32_t frame = 0; frame < frameCount; ++frame) {
        const float sample = static_cast<float>(samples_[static_cast<size_t>(cursor)]) * gain;
        const float clamped = sample > 32767.0f ? 32767.0f : (sample < -32768.0f ? -32768.0f : sample);
        output[frame] = static_cast<int16_t>(clamped);
        cursor += 1;
        if (cursor >= count) {
            cursor = 0;
        }
    }
    cursor_.store(cursor);
}

bool MenuAudio::start(AAssetManager *manager, const char *wavPath, std::string &error) {
    stop();
    std::vector<uint8_t> bytes;
    if (!AssetFile::readBinary(manager, wavPath, bytes, error)) {
        return false;
    }
    if (!readWav(bytes, sampleRate_, samples_, error)) {
        samples_.clear();
        return false;
    }

    AAudioStreamBuilder *builder = nullptr;
    if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK) {
        error = "AAudio builder failed";
        return false;
    }
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, sampleRate_);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_NONE);
    AAudioStreamBuilder_setDataCallback(builder, menuAudioDataCallback, this);
    const aaudio_result_t opened = AAudioStreamBuilder_openStream(builder, &stream_);
    AAudioStreamBuilder_delete(builder);
    if (opened != AAUDIO_OK || stream_ == nullptr) {
        error = "AAudio stream open failed";
        stream_ = nullptr;
        return false;
    }
    if (AAudioStream_requestStart(stream_) != AAUDIO_OK) {
        error = "AAudio stream start failed";
        stop();
        return false;
    }
    playing_ = true;
    aout << "[NG] menu_audio samples=" << samples_.size() << " rate=" << sampleRate_ << std::endl;
    return true;
}

void MenuAudio::stop() {
    playing_ = false;
    if (stream_ != nullptr) {
        AAudioStream_requestStop(stream_);
        AAudioStream_close(stream_);
        stream_ = nullptr;
    }
    samples_.clear();
    cursor_.store(0);
}

void MenuAudio::setGain(float gain) {
    if (gain < 0.0f) {
        gain = 0.0f;
    } else if (gain > 1.0f) {
        gain = 1.0f;
    }
    gain_.store(gain);
}
