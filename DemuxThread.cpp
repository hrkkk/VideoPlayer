#include "DemuxThread.h"


DemuxThread::DemuxThread(PlayerContext* playerCtx) : m_playerCtx(playerCtx)
{}

DemuxThread::~DemuxThread()
{}

void DemuxThread::task()
{
    demux();
    Log("Demux thread exit.");
    m_state = THREAD_STATE::FINISHED;
}

void DemuxThread::demux()
{
    while (true) {       
        if (m_state == THREAD_STATE::STOPPED) {
            break;
        }

        if (m_state == THREAD_STATE::PAUSED) {
            continue;
        }

        std::lock_guard<std::mutex> videoLock(demuxMutex);

        if (m_playerCtx->m_audioQueue.size() >= 100 && m_playerCtx->m_videoQueue.size() >= 100) {
            continue;
        }

        AVPacketPtr packet(av_packet_alloc(), [](AVPacket* ptr) {
            // std::cout << "Packet free" << std::endl;
            av_packet_free(&ptr);
        });

        if (m_playerCtx->m_doSeek) {
            m_playerCtx->m_videoQueue.clear();
            av_seek_frame(m_playerCtx->m_formatCtx, m_playerCtx->m_videoStreamIndex, m_playerCtx->m_seekPosition, AVSEEK_FLAG_BACKWARD);
            m_playerCtx->m_doSeek = false;
        }

        int ret = av_read_frame(m_playerCtx->m_formatCtx, packet.get());
        if (ret == AVERROR_EOF) {
            // std::cout << "File packet read finish." << std::endl;
            // std::cout << "Total Packets: " << totalPackets << std::endl;
            // std::cout << "Videos Packets: " << videoPackets << std::endl;
            // std::cout << "Audios Packets: " << audioPackets << std::endl;
            continue;
            // break;
        } else if (ret < 0) {
            // std::cout << "Read frame error." << std::endl;
            continue;
            // break;
        }
        // std::cout << "Packets: " << (++totalPackets) << std::endl;

        totalPackets.fetch_add(1);

        // 判断是视频包还是音频包
        if (packet->stream_index == m_playerCtx->m_videoStreamIndex) {
            {
                // 添加到视频包队列
                m_playerCtx->m_videoQueue.pushPacket(packet);

                // totalVideos++;
                videoPackets.fetch_add(1);
                // std::cout << "Videos: " << (++totalVideos) << std::endl;

                // cond.notify_one();

                // std::cout << "Send one packet" << packet << std::endl;
            }
        } else if (packet->stream_index == m_playerCtx->m_audioStreamIndex) {
            {
                // 添加到音频包队列
                m_playerCtx->m_audioQueue.pushPacket(packet);

                // totalAudios++;
                audioPackets.fetch_add(1);
                // std::cout << "Audios: " << (++totalAudios) << std::endl;

                // cond.notify_one();

                // std::cout << "Get one packet" << std::endl;
            }
        } else {
            av_packet_unref(packet.get());
        }
    }

    return;
}

