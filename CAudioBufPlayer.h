//
// CAudioBufPlayer.h
//
#ifndef __H_CAudioBufPlayer__
#define __H_CAudioBufPlayer__

#include <audiopolicy.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>

class IAudioSource {
public:
    virtual HRESULT SetFormat(WAVEFORMATEX* pFormat) = 0;
    virtual HRESULT LoadData(UINT32 frameCount, BYTE* pData, DWORD* flags) = 0;
};

class CAudioBufPlayer
{
private:
    IMMDeviceEnumerator* mpEnumerator;
    IMMDevice*          mpDevice;
    IAudioClient*       mpAudioClient;
    WAVEFORMATEX*       mpWaveFormat;
    IAudioRenderClient* mpRenderClient;

    REFERENCE_TIME  mRequestedDuration;
    UINT32          mBufFrameCount;
    IAudioSource*   mDataSource;
    bool mIsPlaying;
    bool mContinueReading;

    void CheckDeviceProperties();
    HRESULT FindAudioDevice(IMMDeviceEnumerator* pEnumerator, IMMDevice** ppDevice, const wchar_t* deviceName);

public:
    CAudioBufPlayer();
	~CAudioBufPlayer();

    bool Init(const wchar_t* deviceName = NULL);
    void Uninit();
    void DoPlaybackLoop();

    bool Start();
    void Stop(bool bWaitToFinish = false);
    bool IsPlaying() { return mIsPlaying; }

    void SetAudioSource(IAudioSource* pSource);
    bool GetWaveFormat(WAVEFORMATEX& format);
};

#endif // __H_CAudioBufPlayer__