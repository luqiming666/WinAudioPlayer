// AudioPeeperLib.cpp : 定义静态库的函数。
//

#include "pch.h"
#include "framework.h"
#include "AudioBufPlayer.h"
#include "Defs.h"
#include <Functiondiscoverykeys_devpkey.h>

#include <iostream>
#include <thread>
#include <chrono>
using namespace std::chrono;

#include <locale>

// https://learn.microsoft.com/en-us/windows/win32/coreaudio/rendering-a-stream
// https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-isformatsupported
////////////////////////////////////////////////////////////////////////////////
CAudioBufPlayer::CAudioBufPlayer() 
    : mpEnumerator(nullptr)
    , mpDevice(nullptr)
    , mpAudioClient(nullptr)
    , mpWaveFormat(nullptr)
    , mpRenderClient(nullptr)
    , mBufFrameCount(0)
    , mRequestedDuration(REFTIMES_PER_SEC)
    , mDataSource(nullptr)
    , mIsPlaying(false)
    , mContinueReading(true)
{
}

CAudioBufPlayer::~CAudioBufPlayer()
{
    Uninit();
}

bool CAudioBufPlayer::Init(const wchar_t* deviceName/*=NULL*/)
{
    HRESULT hr;
    // 获取音频设备枚举器
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&mpEnumerator);
    EXIT_ON_ERROR(hr);

    // 获取音频输出设备
    if (deviceName) {
        hr = FindAudioDevice(mpEnumerator, &mpDevice, deviceName);
    }
    else {
        hr = mpEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mpDevice);
    }
    EXIT_ON_ERROR(hr);
#ifdef DEBUG
    CheckDeviceProperties();
#endif // DEBUG

    // 打开音频输出设备
    hr = mpDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&mpAudioClient);
    EXIT_ON_ERROR(hr);

    // 创建音频客户端
    hr = mpAudioClient->GetMixFormat(&mpWaveFormat);
    EXIT_ON_ERROR(hr);

    //WAVEFORMATEXTENSIBLE* pWaveFormatEx = (WAVEFORMATEXTENSIBLE*)mpWaveFormat;

    /*mpWaveFormat->nSamplesPerSec = 44100;
    mpWaveFormat->wBitsPerSample = 16;
    mpWaveFormat->nBlockAlign = mpWaveFormat->nChannels * mpWaveFormat->wBitsPerSample / 8;
    mpWaveFormat->nAvgBytesPerSec = mpWaveFormat->nBlockAlign * mpWaveFormat->nSamplesPerSec;
    WAVEFORMATEX* pClosestFormat = NULL;
    hr = mpAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, mpWaveFormat, &pClosestFormat);
    if (S_OK == hr) {

    }
    else if (S_FALSE == hr) {

    }
    else if (AUDCLNT_E_UNSUPPORTED_FORMAT == hr) {

    }*/

    // Tell the audio source which format to use.
    if (mDataSource) {
        mDataSource->SetFormat(mpWaveFormat);
    }

    hr = mpAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, mRequestedDuration, 0, mpWaveFormat, nullptr);
    EXIT_ON_ERROR(hr);

    // Get the actual size of the allocated buffer.
    hr = mpAudioClient->GetBufferSize(&mBufFrameCount);
    EXIT_ON_ERROR(hr);

    // 获取音频渲染客户端
    hr = mpAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&mpRenderClient);
    EXIT_ON_ERROR(hr);
    return true;

Exit:
    // 释放资源
    Uninit();
    return false;
}

void CAudioBufPlayer::Uninit()
{
    if (mpWaveFormat) {
        CoTaskMemFree(mpWaveFormat);
        mpWaveFormat = nullptr;
    }
    SAFE_RELEASE(mpRenderClient);
    SAFE_RELEASE(mpAudioClient);
    SAFE_RELEASE(mpDevice);
    SAFE_RELEASE(mpEnumerator);
}

void _PlaybackProc(CAudioBufPlayer* playerInstance)
{
    if (playerInstance) {
        playerInstance->DoPlaybackLoop();
    }
}

bool CAudioBufPlayer::Start()
{
    if (mIsPlaying) return true;

    if (mpAudioClient && mDataSource) {
        mContinueReading = true;

        // 启动一个播放线程
        std::thread myThread(_PlaybackProc, this);
        myThread.detach();

        return true;
    }
    return false;
}

void CAudioBufPlayer::Stop(bool bWaitToFinish)
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

void CAudioBufPlayer::DoPlaybackLoop()
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

bool CAudioBufPlayer::GetWaveFormat(WAVEFORMATEX& format)
{
    if (mpWaveFormat != nullptr) {
        format = *mpWaveFormat;
        return true;
    }
    return false;
}

void CAudioBufPlayer::SetAudioSource(IAudioSource* pSource)
{
    mDataSource = pSource;
}

void CAudioBufPlayer::CheckDeviceProperties()
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

HRESULT CAudioBufPlayer::FindAudioDevice(IMMDeviceEnumerator* pEnumerator, IMMDevice** ppDevice, const wchar_t* deviceName)
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