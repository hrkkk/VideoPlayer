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

#include "PlayerContext.h"

#include <QImage>
#include <QPicture>
#include <QPainter>
#include <QTimer>

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;

std::queue<AVFramePtr> displayQueue;
std::mutex mux;
std::atomic<int> decodeCount(0);
std::atomic<int> displayCount(0);

void start(PlayerContext* playerCtx);
int decodeVideoPacket(PlayerContext* playerCtx);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 创建子线程
    std::thread subThread([]() {
        std::string filePath = "H:/DataCenter/Photo/#2 Photo/other/01e5ae0031d7b48b010376038d2fb17195_258.mp4";
        PlayerContext* playerContext = new PlayerContext();
        playerContext->m_filename = filePath;

        start(playerContext);

        delete playerContext;
    });
    subThread.detach();

    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&]() {
        std::unique_lock<std::mutex> lock(mux);
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

            std::cout << "DisplayCount: " << displayCount << std::endl;
            // std::cout << "Display one frame" << std::endl;
        }
    });
    timer->setInterval(40);
    timer->start();
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
        playerCtx->m_audioSwrCtx = swr_alloc();
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
    // AVPacket* packet = av_packet_alloc();
    AVPacketPtr packet(av_packet_alloc(), [](AVPacket* ptr) {
        std::cout << "Packet free" << std::endl;
        av_packet_free(&ptr);
    });

    while (true) {
        if (av_read_frame(playerCtx->m_formatCtx, packet.get()) < 0) {
            std::cout << "Read frame error." << std::endl;
            break;
        }

        if (packet->stream_index == playerCtx->m_videoStreamIndex) {
            // 添加到视频包队列
            playerCtx->m_videoQueue.pushPacket(packet.get());
            // std::cout << "Read one video frame" << std::endl;

            decodeVideoPacket(playerCtx);
        } else if (packet->stream_index == playerCtx->m_audioStreamIndex) {
            // 添加到音频包队列
            playerCtx->m_audioQueue.pushPacket(packet.get());
        } else {
            av_packet_unref(packet.get());
        }
    }

    // av_packet_free(&packet);

    return 0;
}

int decodeVideoPacket(PlayerContext* playerCtx)
{
    AVCodecContext* codecCtx = playerCtx->m_videoCodecCtx;
    AVFrame* frame = av_frame_alloc();

    while (true) {
        if (playerCtx->m_videoQueue.isEmpty()) {
            break;
        }

        // 从视频队列中获取一包数据
        AVPacket* packet = playerCtx->m_videoQueue.popPacket();

        // 解码该数据包
        int ret = avcodec_send_packet(codecCtx, packet);
        if (ret == 0) {
            ret = avcodec_receive_frame(codecCtx, frame);
        }

        if (ret == 0) {
            decodeCount.fetch_add(1);
            std::cout << "Frame Num: " << decodeCount.load() << std::endl;
            // 创建一个用于存储转换后的 RGB 帧的 AVFrame 对象
            // AVFrame* frameRGB = av_frame_alloc();
            AVFramePtr frameRGBPtr(av_frame_alloc(), [](AVFrame* ptr) {
                av_frame_free(&ptr);
            });
            AVFrame* frameRGB = frameRGBPtr.get();

            // 设置 RGB 帧的参数
            frameRGB->format = AV_PIX_FMT_RGB24; // 设置为 RGB 格式
            frameRGB->width = frame->width;
            frameRGB->height = frame->height;

            // 分配 RGB 帧的数据空间
            ret = av_frame_get_buffer(frameRGB, 0);

            // 执行图像转换
            ret = sws_scale(playerCtx->m_videoSwsCtx, frame->data, frame->linesize, 0, frameRGB->height,
                            frameRGB->data, frameRGB->linesize);

            // 使用转换后的 RGB 帧进行后续处理
            // std::cout << "Decode one video frame" << std::endl;

            {
                std::unique_lock<std::mutex> lock(mux);
                // 将处理后的图像放入显示队列
                displayQueue.push(frameRGBPtr);
            }
        }
    }

    av_frame_free(&frame);
    // av_frame_free(&frameRGB);
    // av_packet_free(&packet);

    return 0;
}

int decodeAudioPacket(PlayerContext* playerCtx)
{
    return 0;
}

void start(PlayerContext* playerCtx)
{
    // 1. 初始化FFmpeg库
    // av_register_all();

    // 2. 打开视频文件
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
    av_dump_format(formatCtx, -1, playerCtx->m_filename.c_str(), 0);
    // std::cout << ss.str() << std::endl;

    if (openStream(playerCtx, AVMEDIA_TYPE_AUDIO) < 0) {
        std::cout << "Open audio stream failed.\r\n";
        return;
    }

    if (openStream(playerCtx, AVMEDIA_TYPE_VIDEO) < 0) {
        std::cout << "Open video stream failed.\r\n";
        return;
    }

    // 开始循环
    getPacket(playerCtx);

    // 3. 获取视频流信息

    // 4. 查找视频流

    // 5. 查找视频解码器

    // 6. 创建解码器上下文

    // 7. 打开解码器


    // 8. 分配帧内存

    // 9. 创建像素格式转换上下文

    // 10. 分配RGB帧内存

    // 11. 解码视频帧

    // 12. 像素格式转换

    // 13. 渲染视频帧

    // 14. 清理资源

}
