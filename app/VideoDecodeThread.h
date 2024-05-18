#ifndef VIDEODECODETHREAD_H
#define VIDEODECODETHREAD_H

#include <mutex>
#include <condition_variable>
#include "PlayerContext.h"
#include "BaseThread.h"
#include "Utils.h"

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;

extern std::mutex demuxMutex;
extern std::mutex videoMutex;
extern std::condition_variable cond;
extern std::atomic<int> videoFrames;
extern std::queue<AVFramePtr> videoFrameQueue;


class VideoDecodeThread : public BaseThread
{
public:
    VideoDecodeThread(PlayerContext* playerCtx);
    ~VideoDecodeThread();

    void task() override;

    void decodeLoop();
    void decodeOnePacket(AVCodecContext* codecCtx, AVFrame* frame);

private:
    PlayerContext* m_playerCtx;
};

#endif // VIDEODECODETHREAD_H
