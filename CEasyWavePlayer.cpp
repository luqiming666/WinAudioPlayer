//
// CEasyWavePlayer.cpp
//

#include "pch.h"
#include "framework.h"
#include "CEasyWavePlayer.h"
#include "Defs.h"
#include <mmsystem.h>

#include <iostream>
#include <thread>
#include <chrono>
using namespace std::chrono;

#include <locale>

////////////////////////////////////////////////////////////////////////////////
CEasyWavePlayer::CEasyWavePlayer() 
    : mpWaveFormat(nullptr)
    , mIsPlaying(false)
    , mIsPaused(false)
    , mContinueReading(true)
{
}

CEasyWavePlayer::~CEasyWavePlayer()
{
    Uninit();
}

bool CEasyWavePlayer::Init(const wchar_t* deviceName/*=NULL*/)
{
    // 枚举音频设备
    UINT deviceCount = waveOutGetNumDevs();
    for (UINT i = 0; i < deviceCount; ++i) {
        WAVEOUTCAPS caps;
        if (waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR) {
            std::cout << "设备ID: " << i << ", 设备名称: " << caps.szPname << std::endl;
        }
    }

Exit:
    // 释放资源
    Uninit();
    return false;
}

void CEasyWavePlayer::Uninit()
{
    if (mpWaveFormat) {
        CoTaskMemFree(mpWaveFormat);
        mpWaveFormat = nullptr;
    }
}

void _PlaybackProc(CEasyWavePlayer* playerInstance)
{
    if (playerInstance) {
        playerInstance->DoPlaybackLoop();
    }
}

// Error codes:
//  0, succeeded
//  -1, playback in progress
//  -2, source file format is not supported 
int CEasyWavePlayer::Play(const wchar_t* sourceFile)
{
    if (mIsPlaying) return -1;

        // 启动一个播放线程
        std::thread myThread(_PlaybackProc, this);
        myThread.detach();

        return true;
    }
    return false;
}

void CEasyWavePlayer::Stop(bool bWaitToFinish)
{
    if (!mIsPlaying) return;

    mContinueReading = false; // Notify the internal thread to terminate
    if (mpAudioClient) {
        mpAudioClient->Stop();
        mpAudioClient->Reset();
    }

    if (bWaitToFinish) {
        while (mIsPlaying) Sleep(10);
    }
}

void CEasyWavePlayer::DoPlaybackLoop()
{
    HRESULT hr;
    BYTE* pData = NULL;
    DWORD flags = 0;
    REFERENCE_TIME hnsActualDuration;
    DWORD halfDuration;
    UINT32 numFramesAvailable, numFramesPadding;

    // See how much buffer space is available.
    hr = mpAudioClient->GetCurrentPadding(&numFramesPadding);
    EXIT_ON_ERROR(hr);

    numFramesAvailable = mBufFrameCount - numFramesPadding;
    if (numFramesAvailable > 0) {
        // Grab the entire buffer for the initial fill operation.
        hr = mpRenderClient->GetBuffer(numFramesAvailable, &pData);
        EXIT_ON_ERROR(hr);

        // Load the initial data into the shared buffer.
        hr = mDataSource->LoadData(numFramesAvailable, pData, &flags);
        EXIT_ON_ERROR(hr);

        hr = mpRenderClient->ReleaseBuffer(numFramesAvailable, flags);
        EXIT_ON_ERROR(hr);
    }

    // Calculate the actual duration of the allocated buffer.
    double ratio = 1.0 * mBufFrameCount / mpWaveFormat->nSamplesPerSec;
    hnsActualDuration = REFTIMES_PER_SEC * ratio;

    hr = mpAudioClient->Start();  // Start playing
    EXIT_ON_ERROR(hr);
    std::cout << ">>> Playback loop started" << std::endl;

    mIsPlaying = true;

    halfDuration = (DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);
    // Each loop fills about half of the shared buffer.
    while (mContinueReading && flags != AUDCLNT_BUFFERFLAGS_SILENT)
    {
        if (mIsPaused) {
            Sleep(10);
            std::cout << "playback loop - paused for 10ms..." << std::endl;
            continue;
        }

        // Sleep for half the buffer duration.
        Sleep(halfDuration);
        std::cout << "playback loop - sleep " << halfDuration << std::endl;

        // See how much buffer space is available.
        hr = mpAudioClient->GetCurrentPadding(&numFramesPadding);
        EXIT_ON_ERROR(hr);
        numFramesAvailable = mBufFrameCount - numFramesPadding;
        std::cout << "be ready to read " << numFramesAvailable << std::endl;

        if (numFramesAvailable > 0) {
            // Grab all the available space in the shared buffer.
            hr = mpRenderClient->GetBuffer(numFramesAvailable, &pData);
            // Get next 1/2-second of data from the audio source.
            hr = mDataSource->LoadData(numFramesAvailable, pData, &flags);
            hr = mpRenderClient->ReleaseBuffer(numFramesAvailable, flags);
            EXIT_ON_ERROR(hr);
        }
    }

    // Wait for last data in buffer to play before stopping.
    if (mContinueReading) {
        Sleep(halfDuration);
    }

Exit:
    mIsPlaying = false;
    if (mpAudioClient) {
        mpAudioClient->Stop();
        mpAudioClient->Reset();
    }
    std::cout << ">>> Playback loop exited" << std::endl;
}

bool CEasyWavePlayer::GetWaveFormat(WAVEFORMATEX& format)
{
    if (mpWaveFormat != nullptr) {
        format = *mpWaveFormat;
        return true;
    }
    return false;
}

void CEasyWavePlayer::SetAudioSource(IAudioSource* pSource)
{
    mDataSource = pSource;
}

void CEasyWavePlayer::CheckDeviceProperties()
{
    HRESULT hr = S_OK;
    IPropertyStore* pProps = NULL;
    LPWSTR pwszID = NULL;

    // Get the endpoint ID string.
    hr = mpDevice->GetId(&pwszID);
    EXIT_ON_ERROR(hr);
    std::wcout << L"Device ID: " << pwszID << std::endl;

    hr = mpDevice->OpenPropertyStore(STGM_READ, &pProps);
    EXIT_ON_ERROR(hr);

    PROPVARIANT varName;
    PropVariantInit(&varName);

    // Get the endpoint's friendly-name property.
    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
    // GetValue succeeds and returns S_OK if PKEY_Device_FriendlyName is not found.
    // In this case vartName.vt is set to VT_EMPTY.      
    if (varName.vt != VT_EMPTY)
    {
        std::wcout << L"Device FriendlyName: " << varName.pwszVal << std::endl;
    }
    PropVariantClear(&varName);

    PropVariantInit(&varName);
    hr = pProps->GetValue(PKEY_Device_DeviceDesc, &varName);
    if (varName.vt != VT_EMPTY)
    {
        std::wcout << L"Device description: " << varName.pwszVal << std::endl;
    }
    PropVariantClear(&varName);

Exit:
    if (pwszID != NULL) {
        CoTaskMemFree(pwszID);
        pwszID = NULL;
    }
    SAFE_RELEASE(pProps);
    
    return;
}

HRESULT CEasyWavePlayer::FindAudioDevice(IMMDeviceEnumerator* pEnumerator, IMMDevice** ppDevice, const wchar_t* deviceName)
{
    IMMDeviceCollection* pDevices = NULL;
    HRESULT hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);

    UINT deviceCount = 0;
    bool bFound = false;
    hr = pDevices->GetCount(&deviceCount);
    if (deviceCount > 0) {
        for (int i = 0; i < deviceCount && !bFound; i++) {
            IMMDevice* pDeviceItem = NULL;
            IPropertyStore* pProps = NULL;
            if (SUCCEEDED(pDevices->Item(i, &pDeviceItem))) {
                hr = pDeviceItem->OpenPropertyStore(STGM_READ, &pProps);

                PROPVARIANT varName;
                PropVariantInit(&varName);
                hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                if (varName.vt != VT_EMPTY)
                {
                    std::wstring strItemName = varName.pwszVal;
                    if (strItemName.compare(deviceName) == 0) {
                        pDeviceItem->AddRef();
                        *ppDevice = pDeviceItem;
                        bFound = true;
                    }
                }
                PropVariantClear(&varName);
            }
            SAFE_RELEASE(pProps);
            SAFE_RELEASE(pDeviceItem);
        }
    }

    return bFound ? S_OK : E_FAIL;
}