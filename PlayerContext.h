#ifndef PLAYERCONTEXT_H
#define PLAYERCONTEXT_H

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#include <string>
#include <queue>

class PacketQueue {
public:
    void pushPacket(AVPacket* packet) {
        m_packetQueue.push(packet);
    }
    AVPacket* popPacket() {
        AVPacket* packet = m_packetQueue.front();
        m_packetQueue.pop();
        return packet;
    }
    bool isEmpty() {
        return m_packetQueue.empty();
    }

private:
    std::queue<AVPacket*> m_packetQueue;
};

class PlayerContext
{
public:
    PlayerContext();
    ~PlayerContext();

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
};

#endif // PLAYERCONTEXT_H
