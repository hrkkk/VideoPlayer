#ifndef DEMUXTHREAD_H
#define DEMUXTHREAD_H

#include <mutex>
#include <condition_variable>
#include "PlayerContext.h"
#include "BaseThread.h"

extern std::mutex demuxMutex;
extern std::condition_variable cond;
extern std::atomic<int> totalPackets;
extern std::atomic<int> videoPackets;
extern std::atomic<int> audioPackets;

class DemuxThread : public BaseThread
{
public:
    DemuxThread(PlayerContext* playerCtx);
    ~DemuxThread();

    void task() override;

    void demux();

private:
    PlayerContext* m_playerCtx;
};

#endif // DEMUXTHREAD_H
