#include "AudioDecodeThread.h"

#include <iostream>

AudioDecodeThread::AudioDecodeThread(PlayerContext* playerCtx) : m_playerCtx(playerCtx)
{}

AudioDecodeThread::~AudioDecodeThread()
{}

void AudioDecodeThread::task()
{
    decode();
}

void AudioDecodeThread::decode()
{
    AVCodecContext* codecCtx = m_playerCtx->m_audioCodecCtx;
    AVFrame* frame = av_frame_alloc();

    while (true) {
        if (m_state == THREAD_STATE::STOPPED) {
            std::cout << "Audio decode thread exit." << std::endl;
            break;
        }

        if (m_state == THREAD_STATE::PAUSED) {
            continue;
        }

        std::unique_lock<std::mutex> lock(mtx);
        // cond.wait(lock, [=]{
        //     return !m_playerCtx->m_audioQueue.isEmpty();
        // });

        if (m_playerCtx->m_audioQueue.isEmpty()) {
            continue;
        }

        if (playQueue.size() >= 100) {
            continue;
        }

        // 从音频队列中取出一包数据
        AVPacketPtr packet = m_playerCtx->m_audioQueue.popPacket();

        // 解码
        int ret = avcodec_send_packet(codecCtx, packet.get());
        if (ret == 0) {
            ret = avcodec_receive_frame(codecCtx, frame);
        }

        // 处理该帧音频
        if (ret == 0) {
            audioFrames.fetch_add(1);

            // std::cout << "Frame sample_rate: " << frame->sample_rate << std::endl;
            // std::cout << "Frame samples: " << frame->nb_samples << std::endl;
            // std::cout << "Frame foramt: " << frame->format << std::endl;
            // 进行音频重采样
            // 为了保持从采样后 dst_nb_samples / dest_sample_rate = src_nb_sample / src_sample_rate
            // 从采样器中会缓存一部分，获取缓存的长度
            int64_t delay = swr_get_delay(m_playerCtx->m_audioSwrCtx, frame->sample_rate);
            int64_t dst_nb_samples = av_rescale_rnd(delay + frame->nb_samples, 44100, frame->sample_rate,
                                                    AV_ROUND_UP);
            // if(nullptr == outFrame) {
            // 创建FramePCM
            AVFramePtr framePCM(av_frame_alloc(), [](AVFrame* ptr) {
                // std::cout << "FramePCM free" << std::endl;
                av_frame_free(&ptr);
            });

            framePCM->sample_rate = 44100;
            framePCM->format = AV_SAMPLE_FMT_S16;
            framePCM->channel_layout = AV_CH_LAYOUT_STEREO;
            framePCM->nb_samples = dst_nb_samples;
            av_frame_get_buffer(framePCM.get(), 0);
            av_frame_make_writable(framePCM.get());

            // 重采样
            swr_convert(m_playerCtx->m_audioSwrCtx, framePCM->data, dst_nb_samples,
                        const_cast<const uint8_t **>(frame->data), frame->nb_samples);

            // std::cout << "FramePCM sample_rate: " << framePCM->sample_rate << std::endl;
            // std::cout << "FramePCM samples: " << framePCM->nb_samples << std::endl;
            // std::cout << "FramePCM foramt: " << framePCM->format << std::endl;

            // 将处理后的音频放入播放队列
            playQueue.push(framePCM);
        }
        av_frame_unref(frame);
    }
    av_frame_free(&frame);

    return;
}

