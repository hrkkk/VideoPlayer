#include "AudioPlayer.h"
#include <iostream>

AudioPlayer::AudioPlayer()
{
    // 初始化SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL can not init: " << SDL_GetError() << std::endl;
        return;
    }

    // 设置音频参数
    SDL_AudioSpec desiredSpec;
    desiredSpec.freq = 48000;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 2;
    desiredSpec.samples = 1024;
    desiredSpec.callback = nullptr;

    // 打开音频设备
    audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desiredSpec, nullptr, 0);
    if (audioDevice == 0) {
        std::cout << "Faile to open audio: " << SDL_GetError() << std::endl;
    }

    // 准备音频数据并播放
    // SDL_QueueAudio(audioDevice, pcmData.data(), pcmData.size());
    SDL_PauseAudioDevice(audioDevice, 0);
}

void AudioPlayer::appendPCMData(AVFrame* audioFrame)
{
    uint8_t* audioData = audioFrame->data[0];
    int dataSize = av_get_bytes_per_sample((AVSampleFormat)audioFrame->format) * audioFrame->nb_samples * audioFrame->channels;

    AudioBuffer buffer;
    buffer.data = audioData;
    buffer.length = dataSize;
    SDL_QueueAudio(audioDevice, buffer.data, buffer.length);

    std::cout << "SDL queue length: " << SDL_GetQueuedAudioSize(audioDevice) << std::endl;
}

void AudioPlayer::play()
{
    SDL_PauseAudioDevice(audioDevice, 0);
}

void AudioPlayer::pause()
{
    SDL_PauseAudioDevice(audioDevice, 1);
}

void AudioPlayer::stop()
{

}
