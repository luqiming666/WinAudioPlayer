//
// CEasyWavePlayer.h
//

#ifndef __H_CEasyWavePlayer__
#define __H_CEasyWavePlayer__

class CEasyWavePlayer
{
private:
    wchar_t             mSourceFile[MAX_PATH];
    WAVEFORMATEX*       mpWaveFormat;

    bool mIsPlaying;
    bool mIsPaused;
    bool mContinueReading;

    void CheckDeviceProperties();

public:
    CEasyWavePlayer();
	~CEasyWavePlayer();

    bool Init(const wchar_t* deviceName = NULL);
    void Uninit();
    void DoPlaybackLoop();

    int Play(const wchar_t* sourceFile);
    void Stop(bool bWaitToFinish = false);
    void Pause() { mIsPaused = true; }
    void Resume() { mIsPaused = false; }
    bool IsPlaying() { return mIsPlaying; }

    bool GetWaveFormat(WAVEFORMATEX& format);
};

#endif // __H_CEasyWavePlayer__