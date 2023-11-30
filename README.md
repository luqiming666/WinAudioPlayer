# WinAudioPlayer
Play audio buffers using Windows Core Audio APIs。
微软官方文档：https://learn.microsoft.com/en-us/windows/win32/coreaudio  

原以为这套 Core Audio APIs 比 waveOutXXX 高级，看起来也确实如此，但事实证明，还是 waveOutXXX 好用！迄今为止，几点总结（未必对）：  

1. Core Audio APIs只能播放 声卡 IAudioClient::GetMixFormat支持的格式，如果这个格式与源文件不同，则要自己进行数据格式的转换。而waveOutOpen可以直接根据源数据格式进行初始化，用起来方便多了。
https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat  

2. 根据微软给的演示代码（Renderig a Stream），写完之后，播放没有声音；而我把喂给声卡的数据dump成本地的一个.wav文件，是可以播放的！>> 2023/11/29 已解决：需要向pRenderClient->GetBuffer的缓存中填充[-1.0, 1.0]的浮点数，而不是整数类型的PCM数据！
https://learn.microsoft.com/en-us/windows/win32/coreaudio/rendering-a-stream
https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat  
