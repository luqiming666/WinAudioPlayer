//
// CEasyWavePlayer.cpp
//

#include "pch.h"
#include "framework.h"
#include "CEasyWavePlayer.h"
#include "Defs.h"
#include <mmsystem.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <thread>

#pragma comment(lib, "winmm.lib")

////////////////////////////////////////////////////////////////////////////////
CEasyWavePlayer::CEasyWavePlayer() 
    : mIsPlaying(false)
    , mDeviceId(0)
{
}

CEasyWavePlayer::~CEasyWavePlayer()
{
}

// What formats does the sound playback device support?
// https://learn.microsoft.com/zh-cn/previous-versions/dd743855(v=vs.85)
// 结论：waveOut api支持的格式挺多的，不过声道数量只支持1个或2个，再多就不行了哦！
void _PrintSupportedFormats(DWORD formats)
{
    // 11.025 kHz
    if ((formats & WAVE_FORMAT_1M08) != 0) {
        std::cout << "11.025 kHz, mono, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_1M16) != 0) {
        std::cout << "11.025 kHz, mono, 16-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_1S08) != 0) {
        std::cout << "11.025 kHz, stereo, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_1S16) != 0) {
        std::cout << "11.025 kHz, stereo, 16-bit" << std::endl;
    }

    // 22.05 kHz
    if ((formats & WAVE_FORMAT_2M08) != 0) {
        std::cout << "22.05 kHz, mono, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_2M16) != 0) {
        std::cout << "22.05 kHz, mono, 16-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_2S08) != 0) {
        std::cout << "22.05 kHz, stereo, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_2S16) != 0) {
        std::cout << "22.05 kHz, stereo, 16-bit" << std::endl;
    }

    // 44.1 kHz
    if ((formats & WAVE_FORMAT_4M08) != 0) {
        std::cout << "44.1 kHz, mono, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_4M16) != 0) {
        std::cout << "44.1 kHz, mono, 16-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_4S08) != 0) {
        std::cout << "44.1 kHz, stereo, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_4S16) != 0) {
        std::cout << "44.1 kHz, stereo, 16-bit" << std::endl;
    }

    // 48 kHz
    if ((formats & WAVE_FORMAT_48M08) != 0) {
        std::cout << "48 kHz, mono, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_48M16) != 0) {
        std::cout << "48 kHz, mono, 16-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_48S08) != 0) {
        std::cout << "48 kHz, stereo, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_48S16) != 0) {
        std::cout << "48 kHz, stereo, 16-bit" << std::endl;
    }

    // 96 kHz
    if ((formats & WAVE_FORMAT_96M08) != 0) {
        std::cout << "96 kHz, mono, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_96M16) != 0) {
        std::cout << "96 kHz, mono, 16-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_96S08) != 0) {
        std::cout << "96 kHz, stereo, 8-bit" << std::endl;
    }
    if ((formats & WAVE_FORMAT_96S16) != 0) {
        std::cout << "96 kHz, stereo, 16-bit" << std::endl;
    }
}

UINT CEasyWavePlayer::GetDevCount()
{
    return waveOutGetNumDevs();
}

void CEasyWavePlayer::CheckDevCaps()
{
    UINT deviceCount = waveOutGetNumDevs();
    for (UINT i = 0; i < deviceCount; ++i) {
        WAVEOUTCAPS caps;
        if (waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR) {
            // 注：设备名字最长只能显示 (32 - 1) 个字符，因此可能显示不全
            std::wcout << L"设备ID: " << i << L", 设备名称: " << caps.szPname << std::endl;

            std::cout << "Supported Formats: " << std::endl;
            _PrintSupportedFormats(caps.dwFormats);
        }
    }
}

void _PlaybackProc(CEasyWavePlayer* playerInstance)
{
    if (playerInstance) {
        playerInstance->DoPlaybackLoop();
    }
}

int CEasyWavePlayer::Play(const wchar_t* sourceFile, UINT deviceId)
{
    if (mIsPlaying) return false;

    wcsncpy_s(mSourceFile, MAX_PATH, sourceFile, _TRUNCATE);
    mDeviceId = deviceId;

    // 启动一个播放线程
    std::thread myThread(_PlaybackProc, this);
    myThread.detach();

    return true;
}

int _ParseWaveFile(const wchar_t* srcFile, WavHeader& header, std::vector<int8_t>& pcmData)
{
    // Codes from ChatGPT 3.5 *_^
    // 打开.wav文件
    std::ifstream file(srcFile, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Failed to open file." << std::endl;
        return 1;
    }

    std::wcout << L"Parsing the wave file: " << srcFile << std::endl;

    // 获取文件大小
    file.seekg(0, std::ios::end);
    int fileSize = (int)file.tellg();
    file.seekg(0, std::ios::beg);

    // 读取文件头
    //WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

    // 检查文件头是否有效
    if (std::string(header.chunkId, 4) != "RIFF" || std::string(header.format, 4) != "WAVE") {
        std::cout << "Invalid WAV file format." << std::endl;
        return 1;
    }

    // 读取PCM数据
    // 注意：header.subchunk2Size不可靠！
    //std::vector<int8_t> pcmData(header.subchunk2Size);
    int bytesToRead = fileSize - sizeof(WavHeader);
    pcmData = std::vector<int8_t>(bytesToRead);
    file.read(reinterpret_cast<char*>(pcmData.data()), bytesToRead);

    // 输出文件信息
    std::cout << "Channels: " << header.numChannels << std::endl;
    std::cout << "Sample Rate: " << header.sampleRate << " Hz" << std::endl;
    std::cout << "Bits per Sample: " << header.bitsPerSample << std::endl;
    std::cout << "PCM Data Size: " << pcmData.size() << " bytes" << std::endl;

    // 关闭文件
    file.close();

    return 0;
}

void CEasyWavePlayer::DoPlaybackLoop()
{
    std::cout << ">>> Playback loop started" << std::endl;
    mIsPlaying = true; // 标识播放正式开始
    HWAVEOUT hWaveOut = NULL;

    WavHeader header;
    std::vector<int8_t> pcmData;
    if (_ParseWaveFile(mSourceFile, header, pcmData) != 0) {
        std::cout << "Failed to open the source file." << std::endl;
        goto Exit;
    }

    WAVEFORMATEX format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = header.numChannels;
    format.nSamplesPerSec = header.sampleRate;
    format.wBitsPerSample = header.bitsPerSample;
    format.nBlockAlign = format.nChannels * (format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = 0;

    // 打开指定的音频设备
    MMRESULT result = waveOutOpen(&hWaveOut, mDeviceId, &format, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
        std::cout << "Failed to open the audio device. Error: " << result << std::endl;
        goto Exit;
    }

    // 检查设备是否支持指定格式
    /*if (!IsDeviceFormatSupported(hWaveOut, &format)) {
        std::cerr << "设备不支持指定的音频格式" << std::endl;
        waveOutClose(hWaveOut);
        file.close();
        return FALSE;
    }*/

    // 创建WAVEHDR结构并填充音频数据
    WAVEHDR waveHdr;
    waveHdr.lpData = reinterpret_cast<char*>(pcmData.data());
    waveHdr.dwBufferLength = pcmData.size();
    waveHdr.dwFlags = 0;
    waveHdr.dwLoops = 0;
    waveHdr.lpNext = NULL;
    waveHdr.reserved = 0;

    // 准备音频数据块
    result = waveOutPrepareHeader(hWaveOut, &waveHdr, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cout << "Failed to prepare audio data. Error: " << result << std::endl;
        goto Exit;
    }

    // 播放音频数据
    result = waveOutWrite(hWaveOut, &waveHdr, sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
        std::cout << "Failed to write audio data. Error: " << result << std::endl;
        goto Exit;
    }

    // 等待播放完成
    while ((waveHdr.dwFlags & WHDR_DONE) == 0) {
        Sleep(10);
    }

Exit:
    // 释放音频数据块
    if (hWaveOut) {
        waveOutUnprepareHeader(hWaveOut, &waveHdr, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
    }    

    mIsPlaying = false;
    std::cout << ">>> Playback loop exited" << std::endl;
}
