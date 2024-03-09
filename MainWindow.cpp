#include "MainWindow.h"
#include "./ui_MainWindow.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}
#include <sstream>
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "PlayerContext.h"
#include "AudioPlayer.h"

#include <QImage>
#include <QPicture>
#include <QPainter>
#include <QTimer>

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;
using PlayerContextPtr = std::shared_ptr<PlayerContext>;

std::queue<AVFramePtr> displayQueue;
std::queue<AVFramePtr> playQueue;
std::mutex mtx;
std::atomic<int> decodeCount(0);
std::atomic<int> displayCount(0);
std::condition_variable cond;

void start(PlayerContext* playerCtx);
int decodeVideoPacket(PlayerContext* playerCtx);
int decodeAudioPacket(PlayerContext* playerCtx);
int getPacket(PlayerContext* playerCtx);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    PlayerContext* playerCtx = new PlayerContext();

    std::string filePath = "H:/DataCenter/Photo/#2 Photo/other/01e5abbe8cd9b177010370038d26e57967_258.mp4";
    playerCtx->m_filename = filePath;
    start(playerCtx);


    // 解封装线程
    std::thread demuxThread([](PlayerContext* playerCtx) {
        getPacket(playerCtx);
    }, playerCtx);
    demuxThread.detach();

    // 视频帧解码线程
    std::thread videoDecodeThread([](PlayerContext* playerCtx) {
        decodeVideoPacket(playerCtx);
    }, playerCtx);
    videoDecodeThread.detach();

    // 音频帧解码线程
    std::thread audioDecodeThread([](PlayerContext* playerCtx) {
        decodeAudioPacket(playerCtx);
    }, playerCtx);
    audioDecodeThread.detach();


    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&]() {
        std::lock_guard<std::mutex> lock(mtx);

        // 如果有待播放的视频帧
        if (!displayQueue.empty()) {
            // 取出图像
            AVFramePtr frame = displayQueue.front();
            displayQueue.pop();

            // 将 AVFrame 转换为 QImage
            QImage image(frame->data[0], frame->width, frame->height, frame->linesize[0], QImage::Format_RGB888);

            // 将 QImage 设置为 QLabel 的图像
            QPixmap pixmap = QPixmap::fromImage(image);
            ui->screen->setPixmap(pixmap.scaled(ui->screen->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

            // 刷新 QLabel
            ui->screen->update();

            displayCount.fetch_add(1);
            ui->decodeNum->setText(QString::number(decodeCount.load()));
            ui->displayNum->setText(QString::number(displayCount.load()));
            ui->progressBar->setValue(displayCount.load());

            // std::cout << "DisplayCount: " << displayCount << std::endl;
            // std::cout << "Display one frame" << std::endl;
        }
    });
    timer->setInterval(40);
    timer->start();


    AudioPlayer* audioPlayer = new AudioPlayer();
    QTimer* audioTimer = new QTimer(this);
    connect(audioTimer, &QTimer::timeout, this, [=]() {
        std::unique_lock<std::mutex> lock(mtx);

        // 如果有待播放的音频帧
        if (!playQueue.empty()) {
            // 取出音频
            AVFramePtr frame = playQueue.front();
            playQueue.pop();

            // 播放
            audioPlayer->appendPCMData(frame.get());
        }
    });
    audioTimer->setInterval(100);
    audioTimer->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void customLogCallback(void* ptr, int level, const char* fmt, va_list vl)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, vl);
    std::stringstream* ss = static_cast<std::stringstream*>(ptr);
    *ss << buf;
}

int openStream(PlayerContext* playerCtx, int mediaType)
{
    AVFormatContext* formatCtx = playerCtx->m_formatCtx;
    AVCodecContext* codecCtx;
    AVCodec* codec;

    // 得到最好的一个流（FFmpeg根据历史经验确定何为最好）对应的解码器
    int streamIndex = av_find_best_stream(formatCtx, (AVMediaType)mediaType, -1, -1, (const AVCodec**)&codec, 0);
    if (streamIndex < 0 || streamIndex >= (int)formatCtx->nb_streams) {
        std::cout << "Can not find a audio stream in the input file\n";
        return -1;
    }
    // 根据得到的解码器创建解码器上下文
    codecCtx = avcodec_alloc_context3(codec);
    // 将format上的关键信息赋值给解码器上下文
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[streamIndex]->codecpar);

    // 打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        std::cout << "Failed to open codec for strem #" << streamIndex << std::endl;
        return -1;
    }

    switch (codecCtx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        playerCtx->m_audioStreamIndex = streamIndex;
        playerCtx->m_audioCodecCtx = codecCtx;
        playerCtx->m_audioStream = formatCtx->streams[streamIndex];
        playerCtx->m_audioSwrCtx = NULL;
        AVChannelLayout outChannel;
        av_channel_layout_default(&outChannel, 2);
        swr_alloc_set_opts2(&playerCtx->m_audioSwrCtx, &outChannel, AV_SAMPLE_FMT_S16, 48000,
                            &codecCtx->ch_layout, codecCtx->sample_fmt, codecCtx->sample_rate, 0, NULL);
        swr_init(playerCtx->m_audioSwrCtx);
        break;
    case AVMEDIA_TYPE_VIDEO:
        playerCtx->m_videoStreamIndex = streamIndex;
        playerCtx->m_videoCodecCtx = codecCtx;
        playerCtx->m_videoStream = formatCtx->streams[streamIndex];
        playerCtx->m_videoSwsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt, codecCtx->width, codecCtx->height,
                                                  AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
        break;
    default:
        break;
    }

    return 0;
}

int getPacket(PlayerContext* playerCtx)
{
    while (true) {
        std::lock_guard<std::mutex> lock(mtx);

        if (playerCtx->m_audioQueue.size() > 50 || playerCtx->m_videoQueue.size() > 50) {
            continue;
        }

        AVPacketPtr packet(av_packet_alloc(), [](AVPacket* ptr) {
            // std::cout << "Packet free" << std::endl;
            av_packet_free(&ptr);
        });

        if (av_read_frame(playerCtx->m_formatCtx, packet.get()) < 0) {
            std::cout << "Read frame error." << std::endl;
            break;
        }

        if (packet->stream_index == playerCtx->m_videoStreamIndex) {
            {
                // 添加到视频包队列
                playerCtx->m_videoQueue.pushPacket(packet);

                cond.notify_one();

                // std::cout << "Send one packet" << packet << std::endl;
            }
        } else if (packet->stream_index == playerCtx->m_audioStreamIndex) {
            {
                // 添加到音频包队列
                playerCtx->m_audioQueue.pushPacket(packet);

                cond.notify_one();

                // std::cout << "Get one packet" << std::endl;
            }
        } else {
            av_packet_unref(packet.get());
        }
    }

    return 0;
}

int decodeVideoPacket(PlayerContext* playerCtx)
{
    AVCodecContext* codecCtx = playerCtx->m_videoCodecCtx;
    AVFrame* frame = av_frame_alloc();

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [=]{
            return !playerCtx->m_videoQueue.isEmpty();
        });

        // 从视频队列中获取一包数据
        AVPacketPtr packet = playerCtx->m_videoQueue.popPacket();

        // 解码该数据包
        int ret = avcodec_send_packet(codecCtx, packet.get());
        if (ret == 0) {
            ret = avcodec_receive_frame(codecCtx, frame);
        }

        // 处理该帧图像
        if (ret == 0) {
            decodeCount.fetch_add(1);
            // std::cout << "Frame Num: " << decodeCount.load() << std::endl;

            // 创建一个用于存储转换后的 RGB 帧的 AVFrame 对象
            AVFramePtr frameRGB(av_frame_alloc(), [](AVFrame* ptr) {
                // std::cout << "FrameRGB free" << std::endl;
                av_frame_free(&ptr);
            });

            // 设置 RGB 帧的参数
            frameRGB->format = AV_PIX_FMT_RGB24; // 设置为 RGB 格式
            frameRGB->width = frame->width;
            frameRGB->height = frame->height;

            // 分配 RGB 帧的数据空间
            av_frame_get_buffer(frameRGB.get(), 0);

            // 执行图像转换
            sws_scale(playerCtx->m_videoSwsCtx, frame->data, frame->linesize, 0, frameRGB->height,
                      frameRGB->data, frameRGB->linesize);

            // 使用转换后的 RGB 帧进行后续处理
            // std::cout << "Decode one video frame" << std::endl;

            {
                // 将处理后的图像放入显示队列
                displayQueue.push(frameRGB);

                // std::cout << "Decode one frame" << std::endl;
            }
        }

        av_frame_unref(frame);
    }

    av_frame_free(&frame);

    return 0;
}

int decodeAudioPacket(PlayerContext* playerCtx)
{
    AVCodecContext* codecCtx = playerCtx->m_audioCodecCtx;
    AVFrame* frame = av_frame_alloc();

    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cond.wait(lock, [=]{
            return !playerCtx->m_audioQueue.isEmpty();
        });

        // 从音频队列中取出一包数据
        AVPacketPtr packet = playerCtx->m_audioQueue.popPacket();

        // 解码
        int ret = avcodec_send_packet(codecCtx, packet.get());
        if (ret == 0) {
            ret = avcodec_receive_frame(codecCtx, frame);
        }

        // 处理该帧音频
        if (ret == 0) {
            // 设置输入和输出参数
            int srcRate = frame->sample_rate;
            int dstRate = 48000;
            int srcChannels = av_get_channel_layout_nb_channels(frame->channel_layout);
            int dstChannels = 2;
            enum AVSampleFormat srcSampleFmt = (AVSampleFormat)frame->format;
            enum AVSampleFormat dstSampleFmt = AV_SAMPLE_FMT_S16;

            // 计算输出的样本数量
            int dstSamplesNum = av_rescale_rnd(swr_get_delay(playerCtx->m_audioSwrCtx, srcRate) + frame->nb_samples,
                                               dstRate,
                                               srcRate,
                                               AV_ROUND_UP);

            // 创建FramePCM
            AVFramePtr framePCM(av_frame_alloc(), [](AVFrame* ptr) {
                std::cout << "FramePCM free" << std::endl;
                av_frame_free(&ptr);
            });

            // 填充输出的Frame
            framePCM->sample_rate = dstRate;
            framePCM->format = dstSampleFmt;
            framePCM->channels = dstChannels;
            framePCM->nb_samples = dstSamplesNum;

            // 分配输出的样本缓冲区
            av_frame_get_buffer(framePCM.get(), 0);

            // 重采样
            swr_convert_frame(playerCtx->m_audioSwrCtx, framePCM.get(), frame);

            // 将处理后的音频放入播放队列
            playQueue.push(framePCM);
        }
        av_frame_unref(frame);
    }
    av_frame_free(&frame);

    return 0;
}

void start(PlayerContext* playerCtx)
{
    AVFormatContext* formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, playerCtx->m_filename.c_str(), NULL, NULL) != 0) {
        qDebug() << "Can not open video file";
        return;
    }

    playerCtx->m_formatCtx = formatCtx;

    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        qDebug() << "Can not find stream information";
        return;
    }

    // 自定义输出流
    // std::stringstream ss;
    // // 设置自定义日志输出回调函数
    // av_log_set_callback(customLogCallback);
    // av_log_set_level(AV_LOG_INFO);
    // 输出音视频文件的详细信息到自定义输出流
    // av_dump_format(formatCtx, -1, playerCtx->m_filename.c_str(), 0);
    // std::cout << ss.str() << std::endl;

    if (openStream(playerCtx, AVMEDIA_TYPE_AUDIO) < 0) {
        std::cout << "Open audio stream failed.\r\n";
        return;
    }

    if (openStream(playerCtx, AVMEDIA_TYPE_VIDEO) < 0) {
        std::cout << "Open video stream failed.\r\n";
        return;
    }
}
