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
#include <iostream>
#include <memory>
#include <mutex>
#include <atomic>
#include <condition_variable>

#include "PlayerContext.h"
#include "DemuxThread.h"
#include "VideoDecodeThread.h"
#include "AudioDecodeThread.h"
#include "AudioPlayThread.h"
#include "VideoPlayThread.h"
#include "Utils.h"

#include <QImage>
#include <QPicture>
#include <QPainter>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidgetItem>
#include <QWidgetAction>
#include <QRadioButton>
#include <QCheckBox>

AVHWDeviceType type;
AVPixelFormat hwPixelFmt;
AVBufferRef* hwDeviceCtx = nullptr;

using AVPacketPtr = std::shared_ptr<AVPacket>;
using AVFramePtr = std::shared_ptr<AVFrame>;
using PlayerContextPtr = std::shared_ptr<PlayerContext>;

std::queue<AVFramePtr> videoFrameQueue;
std::queue<AVFramePtr> audioFrameQueue;

std::mutex demuxMutex;
std::mutex videoMutex;
std::mutex audioMutex;

// 渲染控制
std::atomic<ROTATE_RADIUS> rotateStatus = ROTATE_0;
std::atomic<LAYER_MIX_MODE> layerMixMode = NO_MIX;

std::atomic<int> brightness;
std::atomic<int> saturation;
std::atomic<int> vividness;
std::atomic<int> contrast;
std::atomic<int> colorTemp;
std::atomic<int> sharpness;
std::atomic<int> clarity;


int totalDuration = 0;   // 单位：s
int totalFrames = 0;
std::atomic<int> displayCount(0);
std::atomic<int> playCount(0);

std::atomic<int> totalPackets(0);
std::atomic<int> videoPackets(0);
std::atomic<int> audioPackets(0);
std::atomic<int> videoFrames(0);
std::atomic<int> audioFrames(0);
std::atomic<bool> stopFlag(false);

std::atomic<bool> isMute(false);



void analyzeFile();

PlayerContext* playerCtx = nullptr;
DemuxThread* demuxThread = nullptr;
VideoDecodeThread* videoDecodeThread = nullptr;
AudioDecodeThread* audioDecodeThread = nullptr;
AudioPlayThread* audioPlayThread = nullptr;
VideoPlayThread* videoPlayThread = nullptr;

QTimer* videoPlayTimer = nullptr;

void MainWindow::startWork(const QString& fileName)
{
    // 创建一个播放上下文 PlayerContext
    if (playerCtx == nullptr) {
        playerCtx = PlayerContext::getInstance();
    }

    // 填入媒体文件路径
    if (playerCtx) {
        playerCtx->setFilename(fileName.toStdString());

        // 分析文件，在 UI 界面上填写文件相关信息
        analyzeFile();

        if (m_infoDialog != nullptr) {
            m_infoDialog->setFileInfo(playerCtx);
        }

        // 总时长
        // 将毫秒转化为时间
        int hours = totalDuration / (60 * 60);
        int minutes = (totalDuration % (60 * 60)) / 60;
        int seconds = ((totalDuration % (60 * 60)) % 60);
        ui->duration->setText(QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));


        // 启动线程，开始播放
        // 启动解封装线程
        demuxThread = new DemuxThread(playerCtx);
        demuxThread->start();

        // 如果有视频流
        if (playerCtx->m_streamSet.contains(AVMEDIA_TYPE_VIDEO)) {
            // 启动视频解码线程
            videoDecodeThread = new VideoDecodeThread(playerCtx);
            videoDecodeThread->start();

            // 启动视频播放线程
            videoPlayThread = new VideoPlayThread(playerCtx);
            connect(videoPlayThread, &VideoPlayThread::sig_showImage, ui->openGLWidget, &CustomOpenGLWidget::slot_showYUV);
            connect(videoPlayThread, &VideoPlayThread::sig_currPTS, ui->progressBar, [this](int pts) {
                // 将毫秒转化为时间
                int hours = pts / (60 * 60);
                int minutes = (pts % (60 * 60)) / 60;
                int seconds = ((pts % (60 * 60)) % 60);
                ui->pts->setText(QString("%1:%2:%3").arg(hours, 2, 10, QChar('0')).arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0')));
                // 调整进度条
                double pos = static_cast<double>(pts) / totalDuration;
                ui->progressBar->slotSetValue(pos);
            });
            videoPlayThread->start();
            ui->btn_pause->setIcon(QIcon(":/resource/pause.png"));
        }

        // 如果有音频流
        if (playerCtx->m_streamSet.contains(AVMEDIA_TYPE_AUDIO)) {
            // 启动音频解码线程
            audioDecodeThread = new AudioDecodeThread(playerCtx);
            audioDecodeThread->start();

            // 启动音频播放线程
            audioPlayThread = new AudioPlayThread(playerCtx);
            // audioPlayerThread->setVolume(ui->volume->getPos());
            audioPlayThread->start();
        }
    }
}

void MainWindow::stopWork()
{
    // 停止所有线程，释放所有资源
    if (audioPlayThread != nullptr) {
        audioPlayThread->stop();
        while (!audioPlayThread->isExited()) ;
        delete audioPlayThread;
        audioPlayThread = nullptr;
    }

    if (videoPlayThread != nullptr) {
        videoPlayThread->stop();
        while (!videoPlayThread->isExited()) ;
        delete videoPlayThread;
        videoPlayThread = nullptr;
    }

    if (audioDecodeThread != nullptr) {
        audioDecodeThread->stop();
        while (!audioDecodeThread->isExited()) ;
        delete audioDecodeThread;
        audioDecodeThread = nullptr;
    }

    if (videoDecodeThread != nullptr) {
        videoDecodeThread->stop();
        while (!videoDecodeThread->isExited()) ;
        delete videoDecodeThread;
        videoDecodeThread = nullptr;
    }

    if (demuxThread != nullptr) {
        demuxThread->stop();
        while (!demuxThread->isExited()) ;
        delete demuxThread;
        demuxThread = nullptr;
    }

    if (playerCtx != nullptr) {
        PlayerContext::destroyInstance();
        playerCtx = nullptr;
    }

    // 清空所有变量
    while (!videoFrameQueue.empty()) {
        videoFrameQueue.pop();
    }
    while (!audioFrameQueue.empty()) {
        audioFrameQueue.pop();
    }

    totalFrames = 0;
    displayCount = 0;
    playCount = 0;
    totalPackets = 0;
    videoPackets = 0;
    audioPackets = 0;
    videoFrames = 0;
    audioFrames = 0;

    // 复原 UI 界面
    if (m_infoDialog != nullptr) {
        m_infoDialog->clearFileInfo();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_infoDialog(new InfoDialog(this))
    , m_adjustDialog(new AdjustDialog(this))
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

    connect(ui->btn_more, &QPushButton::clicked, this, [this]() {
        m_infoDialog->show();
    });

    // 设置音量
    ui->volume->slotSetValue(0.5);


    // 音量调节
    connect(ui->volume, &CustomProgressBar::sign_sliderValueChanged, this, [](double pos) {
        if (audioPlayThread != nullptr) {
            audioPlayThread->setVolume(pos * 100);
        }
    });


    connect(ui->btn_pause, &QPushButton::clicked, this, [this]() {
        if (playerCtx == nullptr) {
            return;
        }
        playerCtx->m_isPaused = !playerCtx->m_isPaused;

        if (videoPlayThread != nullptr) {
            if (videoPlayThread->isRunning()) {
                videoPlayThread->pause();
            } else if (videoPlayThread->isPaused()) {
                videoPlayThread->resume();
            }
        }

        if (audioPlayThread != nullptr) {
            if (audioPlayThread->isRunning()) {
                audioPlayThread->pause();
            } else if (audioPlayThread->isPaused()) {
                audioPlayThread->resume();
            }
        }

        if (demuxThread != nullptr) {
            if (demuxThread->isRunning()) {
                demuxThread->pause();
            } else if (demuxThread->isPaused()) {
                demuxThread->resume();
            }
        }

        if (videoDecodeThread != nullptr) {
            if (videoDecodeThread->isRunning()) {
                videoDecodeThread->pause();
            } else if (videoDecodeThread->isPaused()) {
                videoDecodeThread->resume();
            }
        }

        if (audioDecodeThread != nullptr) {
            if (audioDecodeThread->isRunning()) {
                audioDecodeThread->pause();
            } else if (audioDecodeThread->isPaused()) {
                audioDecodeThread->resume();
            }
        }

        if (playerCtx != nullptr) {
            if (playerCtx->m_isPaused) {
                ui->btn_pause->setIcon(QIcon(":/resource/play.png"));
            } else {
                ui->btn_pause->setIcon(QIcon(":/resource/pause.png"));
            }
        }
    });

    // connect(ui->btn_next, &QPushButton::clicked, this, [=]() {
    //     std::lock_guard<std::mutex> lock(mtx);
    //     while (displayQueue.size()) {
    //         displayQueue.pop();
    //     }
    //     playerCtx->seekToPos(3 * AV_TIME_BASE);
    // });

    initMenu();
    connect(ui->btn_setting, &QPushButton::clicked, this, []() {

    });

    connect(ui->btn_mute, &QPushButton::clicked, this, [this]() {
        isMute = !isMute;
        if (isMute) {
            ui->btn_mute->setIcon(QIcon(":/resource/mute.png"));
        } else {
            ui->btn_mute->setIcon(QIcon(":/resource/volume.png"));
        }
    });

    connect(ui->openGLWidget, &CustomOpenGLWidget::sign_mouseClicked, this, [this](int x, int y) {
        m_menu->popup(QPoint(x, y));
    });

    connect(ui->btn_fullScreen, &QPushButton::clicked, this, [this]() {
        emit ui->btn_max->clicked();
    });
}

MainWindow::~MainWindow()
{
    // 停止工作，释放资源
    stopWork();

    delete ui;
}

void MainWindow::initMenu()
{
    m_menu = new QMenu(this);

    // 定义通用的添加Aciton函数
    auto addActionToMenu = [&](QMenu* menu, const QString& actionName, const std::function<void()>& actionFunc) {
        QAction* action = new QAction(actionName, this);
        connect(action, &QAction::triggered, this, actionFunc);
        menu->addAction(action);
    };

    auto addRadionBtnToMenu = [&](QMenu* menu, const QString& text, const std::function<void()>& func, bool defaultChecked = false) {
        QRadioButton* radBtn = new QRadioButton(text);
        radBtn->setChecked(defaultChecked);
        connect(radBtn, &QRadioButton::toggled, this, func);
        QWidgetAction* rotateWidget = new QWidgetAction(menu);
        rotateWidget->setDefaultWidget(radBtn);
        menu->addAction(rotateWidget);
    };


    QMenu* openMenu = new QMenu("打开", this);
    addActionToMenu(openMenu, "打开文件", [&]() {
        QString openFileName = QFileDialog::getOpenFileName(this, "Open File", "C:\\Users\\hrkkk\\Desktop\\", "All Files (*)");
        if (!openFileName.isEmpty()) {
            // 判断文件类型

            // 不是支持的文件类型
            if (false) {
                QMessageBox::warning(this, "Warning", "Unsupported File Type");
                return;
            }
            // 是支持的文件类型 —— 启动播放任务
            startWork(openFileName);
        }
    });

    addActionToMenu(openMenu, "打开链接", [&]() {

    });



    QMenu* imgRotateMenu = new QMenu("图像旋转", this);
    addRadionBtnToMenu(imgRotateMenu, "不旋转", [&]() {
        rotateStatus = ROTATE_RADIUS::ROTATE_0;
    }, true);
    addRadionBtnToMenu(imgRotateMenu, "顺时针旋转90°", [&]() {
        rotateStatus = ROTATE_RADIUS::ROTATE_90;
    });
    addRadionBtnToMenu(imgRotateMenu, "顺时针旋转180°", [&]() {
        rotateStatus = ROTATE_RADIUS::ROTATE_180;
    });
    addRadionBtnToMenu(imgRotateMenu, "顺时针旋转270°", [&]() {
        rotateStatus = ROTATE_RADIUS::ROTATE_270;
    });



    QMenu* imgFlipMenu = new QMenu("图像翻转", this);
    addActionToMenu(imgFlipMenu, "水平翻转", [&]() {

    });
    addActionToMenu(imgFlipMenu, "竖直翻转", [&]() {

    });


    QMenu* imgMixMenu = new QMenu("图层混合", this);
    addRadionBtnToMenu(imgMixMenu, "不混合", [&]() {
        layerMixMode = LAYER_MIX_MODE::NO_MIX;
    }, true);
    addRadionBtnToMenu(imgMixMenu, "正片叠底混合", [&]() {
        layerMixMode = LAYER_MIX_MODE::MULTIPLY;
    });
    addRadionBtnToMenu(imgMixMenu, "滤色混合", [&]() {
        layerMixMode = LAYER_MIX_MODE::SCREEN;
    });
    addRadionBtnToMenu(imgMixMenu, "叠加混合", [&]() {
        layerMixMode = LAYER_MIX_MODE::OVERLAY;
    });
    addRadionBtnToMenu(imgMixMenu, "柔光混合", [&]() {
        layerMixMode = LAYER_MIX_MODE::SOFT_LIGHT;
    });
    addRadionBtnToMenu(imgMixMenu, "强光混合", [&]() {
        layerMixMode = LAYER_MIX_MODE::HARD_LIGHT;
    });


    m_menu->addMenu(openMenu);
    addActionToMenu(m_menu, "关闭", [&]() {
        stopWork();
    });
    m_menu->addMenu(imgRotateMenu);
    m_menu->addMenu(imgFlipMenu);
    addActionToMenu(m_menu, "图像处理", [&]() {
        if (m_adjustDialog != nullptr) {
            m_adjustDialog->show();
        }
    });
    m_menu->addMenu(imgMixMenu);
}

void MainWindow::initFileTree()
{
    FileInfo fileInfo = playerCtx->m_fileInfo;




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
        std::cout << "Can not find a best stream in the input file\n";
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
        // 填充信息
        playerCtx->m_fileInfo.audioCodecName = codecCtx->codec->name;
        playerCtx->m_fileInfo.audioCodecFullName = codecCtx->codec->long_name;
        playerCtx->m_fileInfo.audioDuration = formatCtx->streams[streamIndex]->duration * av_q2d(formatCtx->streams[streamIndex]->time_base);
        playerCtx->m_fileInfo.sampleRate = formatCtx->streams[streamIndex]->codecpar->sample_rate;
        playerCtx->m_fileInfo.channels = formatCtx->streams[streamIndex]->codecpar->ch_layout.nb_channels;
        playerCtx->m_fileInfo.sampleDepth = formatCtx->streams[streamIndex]->codecpar->bits_per_raw_sample;
        playerCtx->m_fileInfo.audioStreamSize = 0;

        // 设置参数
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
        av_opt_set_int(playerCtx->m_audioSwrCtx, "out_sample_rate", 44100, 0);
        av_opt_set_sample_fmt(playerCtx->m_audioSwrCtx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        swr_init(playerCtx->m_audioSwrCtx);
        break;
    case AVMEDIA_TYPE_VIDEO:

        // 填充信息
        playerCtx->m_fileInfo.videoCodecName = codecCtx->codec->name;
        playerCtx->m_fileInfo.videoCodecFullName = codecCtx->codec->long_name;
        playerCtx->m_fileInfo.videoDuration = formatCtx->streams[streamIndex]->duration * av_q2d(formatCtx->streams[streamIndex]->time_base);
        playerCtx->m_fileInfo.width = formatCtx->streams[streamIndex]->codecpar->width;
        playerCtx->m_fileInfo.height = formatCtx->streams[streamIndex]->codecpar->height;
        playerCtx->m_fileInfo.aspectRatio[0] = 0;
        playerCtx->m_fileInfo.aspectRatio[1] = 0;
        playerCtx->m_fileInfo.pixelDepth = formatCtx->streams[streamIndex]->codecpar->bits_per_raw_sample;
        playerCtx->m_fileInfo.colorSpace = "";
        playerCtx->m_fileInfo.chromaSubsampling = "";
        playerCtx->m_fileInfo.frameRate = 0;
        playerCtx->m_fileInfo.videoStreamSize = 0;

        // 设置参数
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


void analyzeFile()
{
    AVFormatContext* formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, playerCtx->m_filename.c_str(), NULL, NULL) != 0) {
        Log("Can not open video file");
        return;
    }

    playerCtx->m_formatCtx = formatCtx;


    if (avformat_find_stream_info(formatCtx, NULL) < 0) {
        Log("Can not find stream information");
        return;
    }

    // 填充文件信息
    playerCtx->m_fileInfo.filename = playerCtx->m_filename;
    playerCtx->m_fileInfo.fileSize = avio_size(formatCtx->pb);
    playerCtx->m_fileInfo.duration = formatCtx->duration / AV_TIME_BASE;
    playerCtx->m_fileInfo.streamNum = formatCtx->nb_streams;
    playerCtx->m_fileInfo.formatName = formatCtx->iformat->name;
    playerCtx->m_fileInfo.formatFullName = formatCtx->iformat->long_name;

    for (unsigned int i = 0; i < formatCtx->nb_streams; ++i) {
        AVStream* stream = formatCtx->streams[i];
        AVMediaType type = stream->codecpar->codec_type;
        playerCtx->m_streamSet.insert(type);
    }

    // 获取视频总时长
    int64_t duration = playerCtx->m_formatCtx->duration;
    totalDuration = duration / AV_TIME_BASE;

    if (playerCtx->m_streamSet.contains(AVMEDIA_TYPE_AUDIO)) {
        if (openStream(playerCtx, AVMEDIA_TYPE_AUDIO) < 0) {
            std::cout << "Open audio stream failed.\r\n";
        }
    }

    if (playerCtx->m_streamSet.contains(AVMEDIA_TYPE_VIDEO)) {
        if (openStream(playerCtx, AVMEDIA_TYPE_VIDEO) < 0) {
            std::cout << "Open video stream failed.\r\n";
        }
    }

    initFilter(playerCtx);

    totalFrames = playerCtx->m_formatCtx->streams[playerCtx->m_videoStreamIndex]->nb_frames;
    // std::cout << totalFrames << std::endl;

}
