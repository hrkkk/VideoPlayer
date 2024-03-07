#include "MainWindow.h"
#include "./ui_MainWindow.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
}
#include <sstream>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    std::string filePath = "H:/DataCenter/Photo/#2 Photo/other/01e5a9d62bd7ff1d010370038d1f6cf309_258.mp4";
    loop(filePath);
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

void openStream(AVFormatContext* formatCtx, int mediaType)
{
    AVCodecContext* codecCtx;
    AVCodec* codec;

    // 得到最好的一个流（根据历史经验确定何为最好）对应的解码器
    int streamIndex = av_find_best_stream(formatCtx, (AVMediaType)mediaType, -1, -1, (const AVCodec**)&codec, 0);
    if (streamIndex < 0 || streamIndex >= (int)formatCtx->nb_streams) {
        std::cout << "Can not find a audio stream in the input file\n";
        return;
    }
    // 根据得到的解码器创建解码器上下文
    codecCtx = avcodec_alloc_context3(codec);
    // 将format上的关键信息赋值给解码器上下文
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[streamIndex]->codecpar);

    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        std::cout << "Failed to open codec for strem #" << streamIndex << std::endl;
        return;
    }

    switch (codecCtx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:

        break;
    case AVMEDIA_TYPE_VIDEO:

        break;
    default:
        break;
    }
}

void MainWindow::loop(std::string fileName)
{
    // 1. 初始化FFmpeg库
    // av_register_all();

    // 2. 打开视频文件
    AVFormatContext* formatCtx = avformat_alloc_context();
    if (avformat_open_input(&formatCtx, fileName.c_str(), NULL, NULL) != 0) {
        qDebug() << "Can not open video file";
        return;
    }

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
    av_dump_format(formatCtx, -1, fileName.c_str(), 0);
    // std::cout << ss.str() << std::endl;

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
