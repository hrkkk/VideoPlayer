#include "AudioPlayThread.h"
#include "Utils.h"
#include <QDebug>
// #include <fstream>

static int volumePercent = SDL_MIX_MAXVOLUME;


// std::ofstream outputFile("audioTimeDiff.csv");
void audio_callback(void* userdata, Uint8* stream, int len)
{
    PlayerContext* playerCtx = (PlayerContext*)userdata;

    // 填充stream数组，填充长度最大为len   len在初始化时根据音频的采样频率、通道数和采样大小计算得出，它表示音频缓冲区的大小（以字节为单位） len = samples * (采样大小 * 声道数)
    // 确保回调函数每次调用时都恰好填充1en字节的数据，以避免音频播放中的咔嗒声或静默。如果提供的数据少于1en字节，SDL会用零填充剩余部分；如果提供的数据多于1en 字节，则超出部分将被忽略。
    SDL_memset(stream, 0, len);

    // uint8_t* buffer = new uint8_t[len];
    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(len);
    size_t bufferSize = 0;

    {
        // 从 PlayerQueue 中取出一帧音频数据
        std::unique_lock<std::mutex> lock(audioMutex);

        if (!audioFrameQueue.empty()) {
            AVFramePtr frame = audioFrameQueue.front();
            audioFrameQueue.pop();

            // 计算当前音频帧的PTS（以秒为单位）
            // audioClock = frame->pts * av_q2d(playerCtx->m_audioStream->time_base) * 1000;
            audioClock = frame->pts;
            // Log("PTS: " + std::to_string(frame->pts));
            // Log("Time base: " + std::to_string(av_q2d(playerCtx->m_audioCodecCtx->time_base)));
            // Log("Audio Clock: " + std::to_string(audioClock));

            if (isMute) return;

            // static long long timestamp = 0;

            // // 获取当前时间点
            // auto now = std::chrono::system_clock::now();
            // // 将当前时间点转换为时间戳（以毫秒为单位）
            // auto currTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

            // int timeDiff = currTimestamp - timestamp;
            // timestamp = currTimestamp;
            // outputFile << std::to_string(timeDiff) << ",";

            bufferSize = frame->linesize[0];
            std::memcpy(buffer.get(), frame->data[0], bufferSize);
        }
    }

    // 音量调节，同时拷贝数据到SDL音频缓冲区
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    int volume = (volumePercent * SDL_MIX_MAXVOLUME) / 100;

    SDL_MixAudioFormat(stream, buffer.get(), AUDIO_S16SYS, bufferSize, volume);
}


AudioPlayThread::AudioPlayThread(PlayerContext* playerCtx) : m_playerCtx(playerCtx)
{}

AudioPlayThread::~AudioPlayThread()
{
    // SDL_CloseAudio();
    // SDL_Quit();
}

void AudioPlayThread::task()
{
    playSound();
}

void AudioPlayThread::playSound()
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "Could not initialize SDL's audio subsystem:%s\n", SDL_GetError());
        return;
    }

    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples = 1024;        // 设置音频缓冲区的大小（以样本为单位，必须为2的n次方   SDL会根据单个样本字节数和通道数来计算缓冲区的实际大小（以字节为单位）
    spec.callback = audio_callback;
    spec.userdata = m_playerCtx;

    // FILE* fp = fopen("C:\\Users\\hrkkk\\Desktop\\ff_output.pcm", "rb");
    // if (fp == nullptr) {
    //     printf("Failed to open PCM file\n");
    //     return;
    // }
    // spec.userdata = fp;

    SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);

    if (device == 0) {
        fprintf(stderr, "Could not open audio: %s\n", SDL_GetError());
        return;
    }

    SDL_PauseAudioDevice(device, 0);

    while (true) {
        if (this->isStopped()) {
            break;
        }

        if (this->isPaused()) {
            SDL_PauseAudioDevice(device, 1);
        } else if (this->isRunning()) {
            SDL_PauseAudioDevice(device, 0);
        } 
    }

    // fclose(fp);
    SDL_CloseAudioDevice(device);
    SDL_Quit();

    this->exit();
    // outputFile.close();
}

// void AudioPlayer::play()
// {
//     m_soundBuffer->Play(0, 0, DSBPLAY_LOOPING);
//     m_isPlaying = true;
// }

// void AudioPlayer::pause()
// {
//     m_soundBuffer->Stop();
//     m_isPlaying = false;
// }

// void AudioPlayer::switchState()
// {
//     if (m_isPlaying) {
//         pause();
//     } else {
//         play();
//     }
// }

// void AudioPlayer::getVolume()
// {
//     // m_soundBuffer->GetVolume();
// }

void AudioPlayThread::setVolume(int volume)
{
    // m_soundBuffer->SetVolume();
    volumePercent = volume;
}

// void AudioPlayer::stop()
// {

// }
