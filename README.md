# WinAudioPlayer
Play audio buffers using Windows Core Audio APIs。  
关于[Windows Core Audio APIs](https://learn.microsoft.com/en-us/windows/win32/coreaudio/about-the-windows-core-audio-apis) ：这套API从Windows Vista开始引入，在Windows 7开始加强。与DirectSound / DirectMusic / waveXxx / mixerXxx / Media Foundation 相比，它是更底层的API。对于大部分音频应用软件，建议使用那些更高层的API。除非你在做这三类应用：Professional audio ("pro audio") applications、Real-time communication (RTC) applications、Third-party audio APIs。
Core Audio APIs有4个部分组成：Multimedia Device (MMDevice) API、Windows Audio Session API (WASAPI)、DeviceTopology API、EndpointVolume API。  


Core Audio APIs是COM编程风格，确实比 waveOutXXX 高级。使用下来的几点总结：  

1. Core Audio APIs只能播放 声卡 [IAudioClient::GetMixFormat](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat) 支持的格式，如果这个格式与源文件不同，则要自己进行数据格式的转换。而waveOutOpen可以直接根据源数据格式进行初始化，用起来方便多了。 
官方论述：[the Core Audio APIs require audio streams to use an audio device's native data formats.](https://learn.microsoft.com/en-us/windows/win32/coreaudio/about-the-windows-core-audio-apis)  

2. 根据微软给的演示代码（[Rendering a Stream](https://learn.microsoft.com/en-us/windows/win32/coreaudio/rendering-a-stream)），写完之后，播放没有声音；而我把喂给声卡的数据dump成本地的一个.wav文件，是可以播放的！>> 2023/11/29 已解决：需要向pRenderClient->GetBuffer的缓存中填充[-1.0, 1.0]的浮点数，而不是整数类型的PCM数据！ 


