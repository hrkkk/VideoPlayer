#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

extern "C" {
#include <SDL.h>
#include <libavcodec/avcodec.h>
}
#include <memory>
#include <mutex>
#include <queue>

using AVFramePtr = std::shared_ptr<AVFrame>;

extern std::mutex mtx;
extern std::queue<AVFramePtr> playQueue;
extern std::atomic<int> playCount;

typedef struct {
    uint8_t* data;
    size_t length;
} AudioBuffer;


class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    void play();
    void pause();
    void stop();
    void switchState();

private:
    SDL_AudioDeviceID m_audioDevice;
    bool m_isPlaying;
};

#endif // AUDIOPLAYER_H
