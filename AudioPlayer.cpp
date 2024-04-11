#include "AudioPlayer.h"
#include <iostream>


void audioCallback(void* userdata, Uint8* stream, int len) {
    int frame_size = 0;
    AVFramePtr frame;

    // Lock the queue and get a frame
    {
        std::unique_lock<std::mutex> lock(mtx);
        if (!playQueue.empty()) {
            frame = playQueue.front();
            playQueue.pop();
            playCount.fetch_add(1);
            frame_size = av_samples_get_buffer_size(nullptr, 2, frame->nb_samples, AV_SAMPLE_FMT_S16, 0);
        }
    }

    if (frame_size > 0 && frame) {
        memcpy(stream, frame->data[0], frame_size);
        // av_frame_free(&frame); // Remember to free the frame
    } else {
        // Fill the buffer with silence if no frame available
        memset(stream, 0, len);
    }
}


AudioPlayer::AudioPlayer()
{
    // 初始化SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL can not init: " << SDL_GetError() << std::endl;
        return;
    }

    // 设置音频参数
    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = 44100;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 2;
    desiredSpec.silence = 0;
    desiredSpec.samples = 2048;
    desiredSpec.callback = audioCallback;

    // 打开音频设备
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, nullptr, 0);
    if (m_audioDevice == 0) {
        std::cout << "Faile to open audio: " << SDL_GetError() << std::endl;
    }

    // 播放
    SDL_PauseAudioDevice(m_audioDevice, 0);
    m_isPlaying = true;
}

AudioPlayer::~AudioPlayer()
{
    // 释放SDL资源
    SDL_PauseAudioDevice(m_audioDevice, 1);
    // SDL_CloseAudioDevice(m_audioDevice);
    // SDL_Quit();
}

void AudioPlayer::play()
{
    SDL_PauseAudioDevice(m_audioDevice, 0);
    m_isPlaying = true;
}

void AudioPlayer::pause()
{
    SDL_PauseAudioDevice(m_audioDevice, 1);
    m_isPlaying = false;
}

void AudioPlayer::switchState()
{
    if (m_isPlaying) {
        pause();
    } else {
        play();
    }
}

void AudioPlayer::stop()
{

}
