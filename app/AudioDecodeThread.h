#ifndef AUDIODECODETHREAD_H
#define AUDIODECODETHREAD_H

#include <mutex>
#include <condition_variable>
#include "PlayerContext.h"
#include "BaseThread.h"

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;

extern std::mutex demuxMutex;
extern std::mutex audioMutex;
extern std::condition_variable cond;
extern std::atomic<int> audioFrames;
extern std::queue<AVFramePtr> audioFrameQueue;

class AudioDecodeThread : public BaseThread
{
public:
    AudioDecodeThread(PlayerContext* playerCtx);
    ~AudioDecodeThread();

    void task() override;

    void decodeLoop();
    void decodeOnePacket(AVCodecContext* codecCtx, AVFrame* frame);

private:
    PlayerContext* m_playerCtx;
};

#endif // AUDIODECODETHREAD_H
