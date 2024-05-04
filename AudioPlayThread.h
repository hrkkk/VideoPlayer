#ifndef AUDIOPLAYTHREAD_H
#define AUDIOPLAYTHREAD_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <SDL.h>
}
#include <memory>
#include <mutex>
#include <queue>
#include "BaseThread.h"
#include "PlayerContext.h"

using AVFramePtr = std::shared_ptr<AVFrame>;

extern std::mutex audioMutex;
extern std::queue<AVFramePtr> audioFrameQueue;
extern std::atomic<int> playCount;
extern std::atomic<double> audioClock;

class AudioPlayThread : public BaseThread {
public:
    AudioPlayThread(PlayerContext* playerCtx);
    ~AudioPlayThread();

    void task() override;
    void playSound();
    // void play();
    // void pause();
    // void stop();
    // void switchState();
    // void getVolume();
    void setVolume(int volume);

private:
    bool m_isPlaying;
    PlayerContext* m_playerCtx;
};

#endif // AUDIOPLAYTHREAD_H
