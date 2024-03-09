#ifndef AUDIOPLAYER_H
#define AUDIOPLAYER_H

extern "C" {
#include <SDL.h>
#include <libavcodec/avcodec.h>
}

typedef struct {
    uint8_t* data;
    size_t length;
} AudioBuffer;

class AudioPlayer {
public:
    AudioPlayer();

    void play();
    void pause();
    void stop();
    void appendPCMData(AVFrame* audioFrame);

private:
    SDL_AudioDeviceID audioDevice;
};

#endif // AUDIOPLAYER_H
