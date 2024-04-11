#include "VideoDecodeThread.h"

#include <iostream>

VideoDecodeThread::VideoDecodeThread(PlayerContext* playerCtx) : m_playerCtx(playerCtx)
{}

VideoDecodeThread::~VideoDecodeThread()
{}

void VideoDecodeThread::task()
{
    decode();
}

void VideoDecodeThread::decode()
{
    AVCodecContext* codecCtx = m_playerCtx->m_videoCodecCtx;
    AVFrame* frame = av_frame_alloc();
#ifdef USE_HARDWARE_DECODE
    AVFrame* swsFrame = av_frame_alloc();
    AVFrame* tmpFrame = nullptr;
#endif

    while (true) {
        if (m_state == THREAD_STATE::STOPPED) {
            std::cout << "Video decode thread exit." << std::endl;
            break;
        }

        if (m_state == THREAD_STATE::PAUSED) {
            continue;
        }

        std::unique_lock<std::mutex> lock(mtx);
        // cond.wait(lock, [=]{
        //     return !m_playerCtx->m_videoQueue.isEmpty();
        // });
        if (m_playerCtx->m_videoQueue.isEmpty()) {
            continue;
        }

        if (displayQueue.size() >= 100) {
            continue;
        }

        // 从视频队列中获取一包数据
        AVPacketPtr packet = m_playerCtx->m_videoQueue.popPacket();

        // 解码该数据包
        int ret = avcodec_send_packet(codecCtx, packet.get());
        if (ret == 0) {
            ret = avcodec_receive_frame(codecCtx, frame);
        }

        // 获取播放时间戳
        double pts = 0;
        if (packet->dts == AV_NOPTS_VALUE && frame->opaque && *(uint64_t*)frame->opaque != AV_NOPTS_VALUE) {
            pts = (double)*(uint64_t*)frame->opaque;
        } else if (packet->dts != AV_NOPTS_VALUE) {
            pts = (double)packet->dts;
        }
        pts *= av_q2d(m_playerCtx->m_videoStream->time_base);

        // 处理该帧图像
        if (ret == 0) {
            // std::cout << "frame format: " << av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)) << std::endl;
            videoFrames.fetch_add(1);
            // std::cout << "Frame Num: " << decodeCount.load() << std::endl;

#ifdef USE_HARDWARE_DECODE
            if (av_hwframe_transfer_data(swsFrame, frame, 0) < 0) {
                std::cout << "Error transferring data to cpu memory.\n";
            }
            if (swsFrame == nullptr) {
                std::cout << "swsFrame is null.\n";
            }
            tmpFrame = swsFrame;
            // std::cout << "tmpFrame format: " << av_get_pix_fmt_name(static_cast<AVPixelFormat>(tmpFrame->format)) << std::endl;
#endif

            // 创建一个用于存储转换后的 RGB 帧的 AVFrame 对象
            AVFramePtr frameRGB(av_frame_alloc(), [](AVFrame* ptr) {
                // std::cout << "FrameRGB free" << std::endl;
                av_frame_free(&ptr);
            });

#ifdef USE_HARDWARE_DECODE
            // 设置 RGB 帧的参数
            frameRGB->format = AV_PIX_FMT_YUV420P; // 设置为 RGB 格式
            frameRGB->width = tmpFrame->width;
            frameRGB->height = tmpFrame->height;

            // 分配 RGB 帧的数据空间
            av_frame_get_buffer(frameRGB.get(), 0);

            // 执行图像转换
            sws_scale(m_playerCtx->m_videoSwsCtx, tmpFrame->data, tmpFrame->linesize, 0, frameRGB->height,
                      frameRGB->data, frameRGB->linesize);


            double frameDelay;

            if (pts != 0) {
                m_playerCtx->m_videoClock = pts;
            } else {
                pts = m_playerCtx->m_videoClock;
            }
            // 更新视频计时器
            frameDelay = av_q2d(m_playerCtx->m_videoCodecCtx->time_base);
            // 如果重复1帧，则相应调整一下时钟
            frameDelay += tmpFrame->repeat_pict * (frameDelay * 0.5);
            m_playerCtx->m_videoClock += frameDelay;
            // 设置帧的PTS
            frameRGB->pts = pts;
            // Log(std::to_string(pts));
#else
            // 设置 RGB 帧的参数
            frameRGB->format = AV_PIX_FMT_RGB24; // 设置为 RGB 格式
            frameRGB->width = frame->width;
            frameRGB->height = frame->height;

            // 分配 RGB 帧的数据空间
            av_frame_get_buffer(frameRGB.get(), 0);

            // 执行图像转换
            sws_scale(m_playerCtx->m_videoSwsCtx, frame->data, frame->linesize, 0, frameRGB->height,
                      frameRGB->data, frameRGB->linesize);


            double frameDelay;

            if (pts != 0) {
                m_playerCtx->m_videoClock = pts;
            } else {
                pts = m_playerCtx->m_videoClock;
            }
            // 更新视频计时器
            frameDelay = av_q2d(m_playerCtx->m_videoCodecCtx->time_base);
            // 如果重复1帧，则相应调整一下时钟
            frameDelay += frame->repeat_pict * (frameDelay * 0.5);
            m_playerCtx->m_videoClock += frameDelay;
            // 设置帧的PTS
            frameRGB->pts = pts;
            Log(std::to_string(pts));
#endif

            // 使用转换后的 RGB 帧进行后续处理
            // std::cout << "Decode one video frame" << std::endl;

            {
                // 将处理后的图像放入显示队列
                displayQueue.push(frameRGB);

                // std::cout << "Decode one frame" << std::endl;
            }
        }
        av_frame_unref(frame);
#ifdef USE_HARDWARE_DECODE
        av_frame_unref(tmpFrame);
#endif
    }

    av_frame_free(&frame);
#ifdef USE_HARDWARE_DECODE
    av_frame_free(&tmpFrame);
#endif

    return;
}

