#ifndef PLAYERCONTEXT_H
#define PLAYERCONTEXT_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
}
#include <string>
#include <queue>
#include <memory>
#include <atomic>
#include "Utils.h"


#define USE_HARDWARE_DECODE

using AVPacketPtr = std::shared_ptr<AVPacket>;

struct FileInfo {
    std::string filename;
    int64_t fileSize;

    int64_t mediaDuration;

    // 视频数据
    int64_t videoDuration;      // 视频时长
    int width;      // 帧宽
    int height;     // 帧高
    int aspectRatio;    // 比例
    int bitDepth;       // 位深
    int colorSpace;     // 色彩空间
    int chromaSubsampling;      // 色度下采样
    double frameRate;   // 帧率

    // 音频数据
    int64_t audioDuration;    // 音频时长
    double sampleRate;      // 采样率
    int channels;       // 通道数

};

class PacketQueue {
public:
    void pushPacket(AVPacketPtr packet) {
        m_packetQueue.push(packet);
    }
    AVPacketPtr popPacket() {
        AVPacketPtr packet = m_packetQueue.front();
        m_packetQueue.pop();
        return packet;
    }
    bool isEmpty() {
        return m_packetQueue.empty();
    }
    size_t size() {
        return m_packetQueue.size();
    }
    void clear() {
        while (m_packetQueue.size()) {
            m_packetQueue.pop();
        }
    }

private:
    std::queue<AVPacketPtr> m_packetQueue;
};

class PlayerContext
{
public:
    PlayerContext();
    ~PlayerContext();

    void setFilename(const std::string& filename);
    void seekToPos(double position);

public:
    std::string m_filename;

    AVFormatContext* m_formatCtx;

    AVCodecContext* m_videoCodecCtx;
    AVCodecContext* m_audioCodecCtx;

    int m_videoStreamIndex;
    int m_audioStreamIndex;

    AVStream* m_videoStream;
    AVStream* m_audioStream;

    PacketQueue m_videoQueue;
    PacketQueue m_audioQueue;

    SwsContext* m_videoSwsCtx;
    SwrContext* m_audioSwrCtx;

    // 音视频同步相关参数
    double m_videoClock = 0.0;
    double m_audioClock = 0.0;
    double m_frameTimer = 0.0;
    double m_frameLastPts = 0.0;
    double m_frameLastDelay = 0.0;

    // seek相关参数
    std::atomic<bool> m_doSeek = false;
    std::atomic<double> m_seekPosition;

    std::atomic<bool> m_isPaused = false;
    std::atomic<bool> m_isStopped = false;

    // 滤镜相关参数
    AVFilterGraph* m_filterGraph;
    AVFilterInOut* m_filterOutputs;
    AVFilterInOut* m_filterInputs;
    AVFilterContext* m_bufferSrcCtx;
    AVFilterContext* m_bufferSinkCtx;
};

#endif // PLAYERCONTEXT_H
