#include "VideoPlayThread.h"
#include "Utils.h"
#include <mutex>
// #include <fstream>

// 音画同步
std::atomic<double> audioClock = 0.0;
std::atomic<double> videoClock = 0.0;

VideoPlayThread::VideoPlayThread(PlayerContext* playerCtx) : m_playerCtx(playerCtx)
{}

void VideoPlayThread::task()
{
    displayLoop();
}

void VideoPlayThread::displayLoop()
{
    while (true) {
        if (this->isStopped()) {
            emit sig_showImage(AVFramePtr(), m_playerCtx->m_videoCodecCtx->width, m_playerCtx->m_videoCodecCtx->height);
            emit sig_currPTS(0);
            break;
        }

        if (this->isFinished() || this->isPaused()) {
            continue;
        }

        if (this->isRunning()) {
            displayOneFrame();
        }
    }

    this->exit();
}

void VideoPlayThread::displayOneFrame()
{
    std::lock_guard<std::mutex> lock(videoMutex);

    // 如果有待播放的视频帧
    if (!videoFrameQueue.empty()) {
        // 取出图像
        AVFramePtr frame = videoFrameQueue.front();
        videoFrameQueue.pop();

        // 音视频同步
        // videoClock = av_q2d(m_playerCtx->m_videoCodecCtx->time_base) * m_playerCtx->m_videoCodecCtx->ticks_per_frame
        //              * 1000 * m_playerCtx->m_videoCodecCtx->frame_num;
        // double duration = av_q2d(m_playerCtx->m_videoCodecCtx->time_base) * m_playerCtx->m_videoCodecCtx->ticks_per_frame * 1000;
        // 计算当前音频帧的PTS（以秒为单位）
        // videoClock = frame->pts * av_q2d(m_playerCtx->m_videoStream->time_base) * 1000;
        videoClock = frame->pts;
        // Log("PTS: " + std::to_string(frame->pts));
        // Log("Time base: " + std::to_string(av_q2d(m_playerCtx->m_videoCodecCtx->time_base)));
        // Log("Video Clock: " + std::to_string(videoClock));
        // Log("Duration: " + std::to_string(duration));

        // outputFile1 << std::to_string(videoClock) << ",";
        // outputFile2 << std::to_string(audioClock) << ",";

        // 显示当前帧
        // qDebug() << "Display frame " << displayCount;
        // 将 Frame 发送到 GPU 进行渲染
        emit sig_showImage(frame, m_playerCtx->m_videoCodecCtx->width, m_playerCtx->m_videoCodecCtx->height);
        emit sig_currPTS(frame->pts / 1000);

        // 计算下一帧的显示时间
        // double duration = 0.001;
        double delay = 0;
        double diff = videoClock - audioClock;
        // if (fabs(diff) <= duration) {   // 如果相差不大，则按照指定帧率继续播放
        //     delay = duration;
        // } else
        if (diff > 0) {   // 如果视频超前了，延迟当前帧的显示时间  diff > 0
            delay = 2 * diff;
        } else if (diff < 0) {  // 如果音频超前了，立即播放下一帧  diff < 0
            delay = 0;
        }

        // static long long timestamp = 0;
        // // 获取当前时间点
        // auto now = std::chrono::system_clock::now();
        // // 将当前时间点转换为时间戳（以毫秒为单位）
        // auto currTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        // int timeDiff = currTimestamp - timestamp;
        // timestamp = currTimestamp;
        // outputFile3 << std::to_string(timeDiff) << ",";


        // 延迟delay毫秒
        std::this_thread::sleep_for(std::chrono::milliseconds((int)delay));


        // // 使用滤镜处理Frame
        // int ret = av_buffersrc_add_frame(playerCtx->m_bufferSrcCtx, frame.get());
        // if (ret < 0) {
        //     std::cout << "add frame error.\n";
        // }
        // AVFrame* filtFrame = av_frame_alloc();
        // ret = av_buffersink_get_frame(playerCtx->m_bufferSinkCtx, filtFrame);
        // if (ret < 0) {
        //     std::cout << "get frame error.\n";
        // }

        // // 将 AVFrame 转换为 QImage
        // QImage image(frame->data[0], frame->width, frame->height, frame->linesize[0], QImage::Format_RGB888);
        // // av_frame_free(&filtFrame);

        // // 将 QImage 设置为QLabel的图像
        // QPixmap pixmap = QPixmap::fromImage(image);
        // ui->screen->setPixmap(pixmap.scaled(ui->screen->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // displayCount.fetch_add(1);

        // double playProgress = static_cast<double>(displayCount * 1.0 / totalFrames);
        // double cacheProgress = static_cast<double>((displayCount + displayQueue.size()) * 1.0 / totalFrames);
        // emit sig_setSlider(playProgress, cacheProgress);
    }
}
