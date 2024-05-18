#ifndef VIDEOPLAYTHREAD_H
#define VIDEOPLAYTHREAD_H

#include "BaseThread.h"
extern "C" {
#include <libavcodec/avcodec.h>
}
#include <QObject>
#include <mutex>
#include <queue>
#include "PlayerContext.h"

using AVFramePtr = std::shared_ptr<AVFrame>;

extern std::mutex videoMutex;
extern std::queue<AVFramePtr> videoFrameQueue;



class VideoPlayThread : public BaseThread
{
    Q_OBJECT
public:
    VideoPlayThread(PlayerContext* playerCtx);

    void task() override;
    void displayLoop();
    void displayOneFrame();

signals:
    void sig_showImage(AVFramePtr ptr, uint width, uint height);
    void sig_setSlider(double playProgress, double cacheProgress);
    void sig_currPTS(int pts);

private:
    PlayerContext* m_playerCtx;
};

#endif // VIDEOPLAYTHREAD_H
