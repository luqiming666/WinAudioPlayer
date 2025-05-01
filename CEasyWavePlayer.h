//
// CEasyWavePlayer.h
//

#ifndef __H_CEasyWavePlayer__
#define __H_CEasyWavePlayer__

class CEasyWavePlayer
{
private:
    wchar_t mSourceFile[MAX_PATH];
    UINT    mDeviceId;

    bool mIsPlaying;

public:
    CEasyWavePlayer();
	~CEasyWavePlayer();

    int Play(const wchar_t* sourceFile, UINT deviceId);
    bool IsPlaying() { return mIsPlaying; }

    UINT GetDevCount();
    void CheckDevCaps();

    void DoPlaybackLoop();
};

#endif // __H_CEasyWavePlayer__