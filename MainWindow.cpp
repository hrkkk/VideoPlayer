#include "MainWindow.h"
#include "./ui_MainWindow.h"


extern "C" {
#include <SDL.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}
#include <sstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <format>

#include "PlayerContext.h"
#include "DemuxThread.h"
#include "VideoDecodeThread.h"
#include "AudioDecodeThread.h"
#include "AudioPlayer.h"
#include "Utils.h"

#include <QImage>
#include <QPicture>
#include <QPainter>
#include <QTimer>

AVHWDeviceType type;
AVPixelFormat hwPixelFmt;
AVBufferRef* hwDeviceCtx = nullptr;

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;
using PlayerContextPtr = std::shared_ptr<PlayerContext>;

std::queue<AVFramePtr> displayQueue;
std::queue<AVFramePtr> playQueue;
std::mutex mtx;
std::condition_variable cond;

int totalFrames = 0;
std::atomic<int> displayCount(0);
std::atomic<int> playCount(0);

std::atomic<int> totalPackets(0);
std::atomic<int> videoPackets(0);
std::atomic<int> audioPackets(0);
std::atomic<int> videoFrames(0);
std::atomic<int> audioFrames(0);
std::atomic<bool> stopFlag(false);
std::string mediaDuration;


void start(PlayerContext* playerCtx);

PlayerContext* playerCtx = nullptr;
DemuxThread* demuxThread = nullptr;
VideoDecodeThread* videoDecodeThread = nullptr;
AudioDecodeThread* audioDecodeThread = nullptr;
AudioPlayer* audioPlayer = nullptr;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowFlag(Qt::FramelessWindowHint);

    connect(ui->btn_min, &QPushButton::clicked, this, [this]() {
        this->showMinimized();
    });
    connect(ui->btn_max, &QPushButton::clicked, this, [this]() {
        if (this->isMaximized()) {
            this->showNormal();
        } else {
            this->showMaximized();
        }
    });
    connect(ui->btn_exit, &QPushButton::clicked, this, [this]() {
        this->close();
    });

    ui->infoArea->hide();
    connect(ui->btn_more, &QPushButton::clicked, this, [this]() {
        if (ui->infoArea->isHidden()) {
            ui->infoArea->show();
        } else {
            ui->infoArea->hide();
        }
    });

    ui->volume->slotSetValue(0.5);

    playerCtx = new PlayerContext();

    std::string filePath = "C:/Users/hrkkk/Desktop/20240401_171348.mp4";
    playerCtx->setFilename(filePath);
    start(playerCtx);

    ui->duration->setText(QString::fromStdString(mediaDuration));

    demuxThread = new DemuxThread(playerCtx);
    demuxThread->start();

    videoDecodeThread = new VideoDecodeThread(playerCtx);
    videoDecodeThread->start();

    audioDecodeThread = new AudioDecodeThread(playerCtx);
    audioDecodeThread->start();

    audioPlayer = new AudioPlayer();

    connect(this, &MainWindow::sig_showImage, ui->openGLWidget, &CustomOpenGLWidget::slot_showYUV);


    connect(ui->btn_pause, &QPushButton::clicked, this, [this]() {
        playerCtx->m_isPaused = !playerCtx->m_isPaused;
        audioPlayer->switchState();
        if (playerCtx->m_isPaused) {
            ui->btn_pause->setIcon(QIcon(":/resource/play.png"));
        } else {
            ui->btn_pause->setIcon(QIcon(":/resource/pause.png"));
        }
    });

    connect(ui->btn_next, &QPushButton::clicked, this, [=]() {
        std::lock_guard<std::mutex> lock(mtx);
        while (displayQueue.size()) {
            displayQueue.pop();
        }
        playerCtx->seekToPos(3 * AV_TIME_BASE);
    });

    initMenu();
    connect(ui->btn_setting, &QPushButton::clicked, this, [this]() {
        m_menu->show();
    });

    connect(ui->btn_fullScreen, &QPushButton::clicked, this, [this]() {
        emit ui->btn_max->clicked();
    });


    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [&]() {
        std::lock_guard<std::mutex> lock(mtx);

        ui->totalPackets->setText(QString::number(totalPackets));
        ui->videoPackets->setText(QString::number(videoPackets));

        ui->audioPackets->setText(QString::number(audioPackets));
        ui->videoFrames->setText(QString::number(videoFrames));
        ui->audioFrames->setText(QString::number(audioFrames));
        ui->displayFrames->setText(QString::number(displayCount));
        ui->playFrames->setText(QString::number(playCount));

        if (playerCtx->m_isPaused) {
            return;
        }

        // 如果有待播放的视频帧
        if (!displayQueue.empty()) {

            // 取出图像
            AVFramePtr frame = displayQueue.front();
            displayQueue.pop();

            // // 音视频同步
            // double currentPts = frame->pts;
            // double sec = currentPts * av_q2d(playerCtx->m_formatCtx->streams[playerCtx->m_videoStreamIndex]->time_base);
            // Log(std::to_string(currentPts));
            // double delay = currentPts - playerCtx->m_frameLastPts;
            // if (delay <= 0 || delay >= 10.0) {
            //     delay = playerCtx->m_frameLastDelay;
            // }
            // playerCtx->m_frameLastDelay = delay;
            // playerCtx->m_frameLastPts = currentPts;

            // double refClock = getAudioClock(playerCtx);
            // double diff = currentPts - refClock;

            // const double AV_SYNC_THRESHOLD = 0.01;
            // const double AV_NOSYNC_THRESHOLD = 10.0;
            // double syncThreshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;

            // if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
            //     if (diff < -syncThreshold) {
            //         delay = 0;
            //     } else if (diff >= syncThreshold) {
            //         delay = 2 * delay;
            //     }
            // }

            // playerCtx->m_frameTimer += delay;
            // double actualDelay = playerCtx->m_frameTimer - (av_gettime() / AV_TIME_BASE);
            // if (actualDelay < 0.010) {
            //     actualDelay = 0.010;
            // }

            // 使用滤镜处理Frame
            // int ret = av_buffersrc_add_frame(playerCtx->m_bufferSrcCtx, frame.get());
            // if (ret < 0) {
            //     std::cout << "add frame error.\n";
            // }
            // AVFrame* filtFrame = av_frame_alloc();
            // ret = av_buffersink_get_frame(playerCtx->m_bufferSinkCtx, filtFrame);
            // if (ret < 0) {
            //     std::cout << "get frame error.\n";
            // }

            // qDebug() << "Display frame " << displayCount;
            // 将 Frame 发送到 GPU 进行渲染
            emit sig_showImage(frame, playerCtx->m_videoCodecCtx->width, playerCtx->m_videoCodecCtx->height);


            // // 将 AVFrame 转换为 QImage
            // QImage image(frame->data[0], frame->width, frame->height, frame->linesize[0], QImage::Format_RGB888);
            // // av_frame_free(&filtFrame);

            // // 将 QImage 设置为QLabel的图像
            // QPixmap pixmap = QPixmap::fromImage(image);
            // ui->screen->setPixmap(pixmap.scaled(ui->screen->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

            displayCount.fetch_add(1);

            double playProgress = static_cast<double>(displayCount * 1.0 / totalFrames);
            double cacheProgress = static_cast<double>((displayCount + displayQueue.size()) * 1.0 / totalFrames);
            ui->progressBar->slotSetValue(playProgress, cacheProgress);

            // std::cout << "DisplayCount: " << displayCount << std::endl;
            // std::cout << "Display one frame" << std::endl;
        }
    });
    timer->setInterval(40);
    timer->start();
}

std::atomic_flag flag = ATOMIC_FLAG_INIT;
volatile bool stopp = false;
MainWindow::~MainWindow()
{
    // 停止所有子线程
    // playerCtx->m_isStopped = true;

    if (audioDecodeThread != nullptr) {
        audioDecodeThread->stop();
        delete audioDecodeThread;
        audioDecodeThread = nullptr;
    }

    if (videoDecodeThread != nullptr) {
        videoDecodeThread->stop();
        delete videoDecodeThread;
        videoDecodeThread = nullptr;
    }

    if (demuxThread != nullptr) {
        demuxThread->stop();
        delete demuxThread;
        demuxThread = nullptr;
    }

    if (playerCtx != nullptr) {
        delete playerCtx;
        playerCtx = nullptr;
    }

    if (audioPlayer != nullptr) {
        delete audioPlayer;
        audioPlayer = nullptr;
    }

    delete ui;
}

void MainWindow::initMenu()
{
    m_menu = new QMenu(this);

    QMenu* imgRotateMenu = new QMenu("图像旋转", this);
    QAction* rotate90 = new QAction("顺时针旋转90°", this);
    imgRotateMenu->addAction(rotate90);

    QMenu* imgFlipMenu = new QMenu("图像翻转", this);
    QAction* horFlip = new QAction("左右翻转", this);
    QAction* verFlip = new QAction("上下翻转", this);
    imgFlipMenu->addAction(horFlip);
    imgFlipMenu->addAction(verFlip);

    m_menu->addMenu(imgRotateMenu);
    m_menu->addMenu(imgFlipMenu);
}

void customLogCallback(void* ptr, int level, const char* fmt, va_list vl)
{
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, vl);
    std::stringstream* ss = static_cast<std::stringstream*>(ptr);
    *ss << buf;
}

AVPixelFormat getHwFmtCallback(AVCodecContext* ctx, const AVPixelFormat* pixFmt)
{
    const AVPixelFormat* p;

    for (p = pixFmt; *p != -1; ++p) {
        if (*p == hwPixelFmt) {
            return *p;
        }
    }

    std::cout << "Failed to get HW surface format.\n";
    return AV_PIX_FMT_NONE;
}

void initFilter(PlayerContext* playerCtx)
{
    int ret = 0;
    // 创建滤镜图实例
    playerCtx->m_filterGraph = avfilter_graph_alloc();
    // 获取滤镜处理源和输出
    const AVFilter* bufferSrc = avfilter_get_by_name("buffer");
    const AVFilter* bufferSink = avfilter_get_by_name("buffersink");
    // 初始化滤镜输入和输出
    playerCtx->m_filterOutputs = avfilter_inout_alloc();
    playerCtx->m_filterInputs = avfilter_inout_alloc();

    if (!playerCtx->m_filterGraph || !playerCtx->m_filterInputs || !playerCtx->m_filterOutputs) {
        ret = AVERROR(ENOMEM);
        std::cout << ret << std::endl;
    }
    // 准备filter参数
    char args[512] = {};
    AVRational timeBase = playerCtx->m_formatCtx->streams[playerCtx->m_videoStreamIndex]->time_base;
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             playerCtx->m_videoCodecCtx->width, playerCtx->m_videoCodecCtx->height,
             AV_PIX_FMT_YUV420P,
             timeBase.num, timeBase.den,
             playerCtx->m_videoCodecCtx->sample_aspect_ratio.num, playerCtx->m_videoCodecCtx->sample_aspect_ratio.den);
    std::cout << args << std::endl;
    // 根据参数创建输入滤镜的上下文，并添加到滤镜图
    ret = avfilter_graph_create_filter(&playerCtx->m_bufferSrcCtx, bufferSrc, "in", args, nullptr, playerCtx->m_filterGraph);
    if (ret < 0) {
        std::cout << "Cannot create buffer src. " << ret << "\n";
    }
    // 根据参数创建输入滤镜的上下文，并添加到滤镜图
    ret = avfilter_graph_create_filter(&playerCtx->m_bufferSinkCtx, bufferSink, "out", nullptr, nullptr, playerCtx->m_filterGraph);
    if (ret < 0) {
        std::cout << "Cannot create buffer sink.\n";
    }
    // 设置其他参数
    AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    ret = av_opt_set_int_list(playerCtx->m_bufferSinkCtx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        std::cout << "Cannot set output pixel format.\n";
    }
    // 建立滤镜解析器
    const char* filter_desc = "hflip";
    // 为输入滤镜关联滤镜名
    playerCtx->m_filterOutputs->name = av_strdup("in");
    playerCtx->m_filterOutputs->filter_ctx = playerCtx->m_bufferSrcCtx;
    playerCtx->m_filterOutputs->pad_idx = 0;
    playerCtx->m_filterOutputs->next = nullptr;
    // 为输出滤镜关联滤镜名
    playerCtx->m_filterInputs->name = av_strdup("out");
    playerCtx->m_filterInputs->filter_ctx = playerCtx->m_bufferSinkCtx;
    playerCtx->m_filterInputs->pad_idx = 0;
    playerCtx->m_filterInputs->next = nullptr;
    // 解析Filter字符串，建立Filter间的连接
    avfilter_graph_parse_ptr(playerCtx->m_filterGraph, filter_desc, &playerCtx->m_filterInputs, &playerCtx->m_filterOutputs, nullptr);
    // 配置滤镜图
    avfilter_graph_config(playerCtx->m_filterGraph, nullptr);
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

#ifdef USE_HARDWARE_DECODE
    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        type = av_hwdevice_find_type_by_name("dxva2");
        if (type == AV_HWDEVICE_TYPE_NONE) {
            std::cout << "Device type dxva2 is not supported.\n";
            std::cout << "Available device types:";
            while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE) {
                std::cout << " " << av_hwdevice_get_type_name(type);
            }
            std::cout << std::endl;
        }

        for (int i = 0;; ++i) {
            const AVCodecHWConfig* config = avcodec_get_hw_config(codec, i);
            if (!config) {
                std::cout << "Decoder " << codec->name << " does not support device type " << av_hwdevice_get_type_name(type) << ".\n";
            }
            if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX && config->device_type == type) {
                hwPixelFmt = config->pix_fmt;
                break;
            }
        }
    }
#endif

    // 根据得到的解码器创建解码器上下文
    codecCtx = avcodec_alloc_context3(codec);
    // 将format上的关键信息赋值给解码器上下文
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[streamIndex]->codecpar);

#ifdef USE_HARDWARE_DECODE
    if (mediaType == AVMEDIA_TYPE_VIDEO) {
        codecCtx->get_format = getHwFmtCallback;

        if (av_hwdevice_ctx_create(&hwDeviceCtx, type, NULL, NULL, 0) < 0) {
            std::cout << "Failed to create specified HW device.\n";
            return -1;
        }

        codecCtx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
    }
#endif

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
        av_opt_set_chlayout(playerCtx->m_audioSwrCtx, "in_chlayout", &codecCtx->ch_layout, 0);
        av_opt_set_int(playerCtx->m_audioSwrCtx, "in_sample_rate", codecCtx->sample_rate, 0);
        av_opt_set_sample_fmt(playerCtx->m_audioSwrCtx, "in_sample_fmt", codecCtx->sample_fmt, 0);
        AVChannelLayout outLayout;
        av_channel_layout_default(&outLayout, 2);
        av_opt_set_chlayout(playerCtx->m_audioSwrCtx, "out_chlayout", &outLayout, 0);
        av_opt_set_int(playerCtx->m_audioSwrCtx, "out_sample_rate", 48000, 0);
        av_opt_set_sample_fmt(playerCtx->m_audioSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        swr_init(playerCtx->m_audioSwrCtx);
        break;
    case AVMEDIA_TYPE_VIDEO:
        playerCtx->m_videoStreamIndex = streamIndex;
        playerCtx->m_videoCodecCtx = codecCtx;
        playerCtx->m_videoStream = formatCtx->streams[streamIndex];
#ifdef USE_HARDWARE_DECODE
        playerCtx->m_videoSwsCtx = sws_getContext(codecCtx->width, codecCtx->height, AV_PIX_FMT_NV12, codecCtx->width, codecCtx->height,
                         AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
#else
        playerCtx->m_videoSwsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt, codecCtx->width, codecCtx->height,
                                                  AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
#endif

        break;
    default:
        break;
    }

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

    int64_t duration = playerCtx->m_formatCtx->duration;
    int hours = (duration / (AV_TIME_BASE * (long long)3600));
    int minutes = (duration % (AV_TIME_BASE * (long long)3600)) / (AV_TIME_BASE * 60);
    int seconds = (duration % (AV_TIME_BASE * (long long)60)) / AV_TIME_BASE;

    mediaDuration = std::to_string(hours) + ":" + std::to_string(minutes) + ":" + std::to_string(seconds);

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

    initFilter(playerCtx);

    totalFrames = playerCtx->m_formatCtx->streams[playerCtx->m_videoStreamIndex]->nb_frames;
    std::cout << totalFrames << std::endl;
}
