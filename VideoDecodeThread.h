#ifndef VIDEODECODETHREAD_H
#define VIDEODECODETHREAD_H

#include <mutex>
#include <condition_variable>
#include "PlayerContext.h"
#include "BaseThread.h"
#include "Utils.h"

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;

extern std::mutex mtx;
extern std::condition_variable cond;
extern std::atomic<int> videoFrames;
extern std::queue<AVFramePtr> displayQueue;


class VideoDecodeThread : public BaseThread
{
public:
    VideoDecodeThread(PlayerContext* playerCtx);
    ~VideoDecodeThread();

    void task() override;

    void decode();

private:
    PlayerContext* m_playerCtx;
};

#endif // VIDEODECODETHREAD_H
