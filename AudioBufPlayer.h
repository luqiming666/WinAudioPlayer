//
// AudioBufPlayer.h
//
#ifndef __H_AudioBufPlayer__
#define __H_AudioBufPlayer__

#include <Mmdeviceapi.h>
#include <Audioclient.h>

class IAudioSource {
public:
    virtual void SetFormat(WAVEFORMATEX* pFormat) = 0;
    virtual HRESULT LoadData(UINT32 frameCount, BYTE* pData, DWORD* flags) = 0;
};

class CAudioBufPlayer
{
private:
    IMMDeviceEnumerator* mpEnumerator;
    IMMDevice* mpDevice;
    IAudioClient* mpAudioClient;
    WAVEFORMATEX* mpWaveFormat;
    IAudioRenderClient* mpRenderClient;

    REFERENCE_TIME mRequestedDuration;
    UINT32 mBufFrameCount;
    IAudioSource* mDataSource;
    bool mIsPlaying;
    bool mContinueReading;

public:
    CAudioBufPlayer();
	~CAudioBufPlayer();

    bool Init(IAudioSource* pSource);
    void Uninit();
    void DoPlaybackLoop();

    bool Start();
    void Stop(bool bWaitToFinish = false);
    bool IsPlaying() { return mIsPlaying; }

    void SetAudioSource(IAudioSource* pSource);
    bool GetWaveFormat(WAVEFORMATEX& format);
};

#endif // __H_AudioBufPlayer__