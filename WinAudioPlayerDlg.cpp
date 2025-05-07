
// WinAudioPlayerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "WinAudioPlayer.h"
#include "WinAudioPlayerDlg.h"
#include "afxdialogex.h"
#include "Defs.h"
#include "UMiscUtils.h"

#include "CEasyWavePlayer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

ma_decoder maDecoder;
ma_device maDevice;


//#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
#include "minimp3_ex.h"

#define WM_AUDIO_DEVICE_CHANGED    (WM_USER+100)

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 打开一个控制台窗口，方便查看调试信息
#define DEBUG_WINDOW_IS_OPEN 1

//#define _DUMP_PCM_FILE

#define DUMP_FILENAME _T("D:\\audiodump.wav")

// Dump到本地的PCM - 播放指令：ffplay.exe -ar 48000 -ac 2 -f s32le -i d:\audiodump.pcm
#ifdef _DUMP_PCM_FILE
std::fstream dumpFile;
UINT32 dumpBytes = 0;
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CWinAudioPlayerDlg 对话框

CWinAudioPlayerDlg::CWinAudioPlayerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WINAUDIOPLAYER_DIALOG, pParent)
	, mSourceFile(_T(""))
	, mRequiredFormat(NULL)
	, mIsSynth(FALSE)
	, mbUseFFmpeg(FALSE)
	, mCacheFile(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinAudioPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SOURCE_FILE, mSourceFile);
	DDX_Control(pDX, IDC_COMBO_SOUND_CARDS, mSoundCardList);
	DDX_Check(pDX, IDC_CHECK_SYNTH, mIsSynth);
	DDX_Check(pDX, IDC_CHECK_USE_FFMPEG, mbUseFFmpeg);
}

BEGIN_MESSAGE_MAP(CWinAudioPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSER, &CWinAudioPlayerDlg::OnBnClickedButtonBrowser)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CWinAudioPlayerDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CWinAudioPlayerDlg::OnBnClickedButtonStop)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_AUDIO_DEVICE_CHANGED, OnAudioDeviceChangedMessage)
	ON_CBN_SELCHANGE(IDC_COMBO_SOUND_CARDS, &CWinAudioPlayerDlg::OnCbnSelchangeComboSoundCards)
	ON_BN_CLICKED(IDC_BUTTON_PLAY_WITH_MINIAUDIO, &CWinAudioPlayerDlg::OnBnClickedButtonPlayWithMiniaudio)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, &CWinAudioPlayerDlg::OnBnClickedButtonPause)
	ON_BN_CLICKED(IDC_BUTTON_PLAY_WITH_LEGACY_WAVEOUT, &CWinAudioPlayerDlg::OnBnClickedButtonPlayWithLegacyWaveout)
END_MESSAGE_MAP()


// CWinAudioPlayerDlg 消息处理程序

BOOL CWinAudioPlayerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
#if DEBUG_WINDOW_IS_OPEN
	// 打开一个控制台查看日志
	if (::GetConsoleWindow() == NULL)
	{
		if (::AllocConsole())
		{
			FILE* stream;
			freopen_s(&stream, "CONOUT$", "w", stdout);
		}
	}
#endif

	// 默认情况下，std::cout使用多字节字符集（如ASCII）进行输出，而不是宽字符集（如Unicode）
	// 设置std::cout的本地化为宽字符输出
	std::locale::global(std::locale(""));
	// 使用std::wcout输出宽字符
	std::wcout.imbue(std::locale());

	// 初始化COM组件
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	mAudioPlayer.SetAudioSource(this);
	mAudioPlayer.SetDeviceListener(this);
	mAudioPlayer.Init(); // init with the default device

	// 枚举所有音频播放设备
	std::vector<std::wstring> soundCards = UMiscUtils::GetAllSoundDevices();
	for (std::wstring& item : soundCards) {
		mSoundCardList.AddString(item.c_str());
	}
	mSoundCardList.SelectString(-1, UMiscUtils::GetDefaultSoundDevice().c_str()); // Select the default device

	TryToPlayFromCommandline();

	// Set up FFmpeg
	mMpegHub.SetTaskObserver(this);
	mMpegHub.LocateTools(UMiscUtils::GetRuntimeFilePath());

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CWinAudioPlayerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWinAudioPlayerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWinAudioPlayerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWinAudioPlayerDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	mAudioPlayer.Stop(true);

	// Release MiniAudio resources
	MA_Stop();

	// 反初始化COM组件
	CoUninitialize();

	if (!mCacheFile.IsEmpty()) {
		DeleteFile(mCacheFile);
		mCacheFile.Empty();
	}
}


WavHeader header;
std::vector<int8_t> pcmData;

int parseWaveFile(const wchar_t* srcFile)
{
	// Codes from ChatGPT 3.5 *_^
	// 打开.wav文件
	std::ifstream file(srcFile, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Failed to open file." << std::endl;
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
		std::cerr << "Invalid WAV file format." << std::endl;
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

bool isMP3File(const CString& fileName) {
	int dotPos = fileName.ReverseFind('.');
	if (dotPos != -1) {
		CString extension = fileName.Mid(dotPos + 1);
		return extension.CompareNoCase(_T("mp3")) == 0;
	}
	return false;
}

void CWinAudioPlayerDlg::OnBnClickedButtonBrowser()
{
	UpdateData(TRUE);

	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Wave Files (*.wav)|*.wav|MP3 Files (*.mp3)|*.mp3|OGG Files (*.ogg)|*.ogg|All Files (*.*)|*.*||"), NULL);
	if (fileDlg.DoModal() == IDOK)
	{
		mSourceFile = fileDlg.GetPathName();
		UpdateData(FALSE);

		// Use FFmpeg for audio format conversion
		if (mbUseFFmpeg) {
			std::string cacheFilename = UMiscUtils::generateRandomFileName(".wav");
			wchar_t* pFilename = UMiscUtils::AtoW(cacheFilename.c_str());
			mCacheFile = UMiscUtils::GetProgramDataPath(_T("MyFFmpeg"), pFilename); // 生成一个临时文件
			delete[] pFilename;

			// [TBD] 是否指定audio codec: -acodec pcm_u8 / pcm_s16le / pcm_f32le ?
			CString strCmd;
			strCmd.Format(_T(" -i %s -vn -ar %d -ac %d -y %s"), (LPCTSTR)mSourceFile, mRequiredFormat->nSamplesPerSec, mRequiredFormat->nChannels, (LPCTSTR)mCacheFile); // 注意：-i之前须有一个空格
			mMpegHub.Run(strCmd);
			this->GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(FALSE); // 转换完成前不允许播放
			return;
		}
		else {
			// 如果源文件是MP3，则先进行解码
			if (isMP3File(mSourceFile)) {
				DecodeMp3ToPcmBuffer();
			}
			else {
				parseWaveFile((LPCTSTR)mSourceFile);
			}
		}				

		// 如果源文件的采样频率与设备要求不一致，则进行重采样
		// 否则，播放节奏不对！！！
		if (header.sampleRate != mRequiredFormat->nSamplesPerSec) {
			// Not good!
			//pcmData = UMiscUtils::Resample(pcmData, header.bitsPerSample, header.sampleRate, mRequiredFormat->nSamplesPerSec);

			uint64_t inFrames = pcmData.size() / header.bitsPerSample * 8 / header.numChannels;
			std::vector<int8_t> outPcmBuf;
			uint64_t outBytes = (uint64_t)(pcmData.size() * (double)mRequiredFormat->nSamplesPerSec / (double)header.sampleRate);
			outBytes -= outBytes % header.numChannels;
			outPcmBuf.resize(outBytes);

			if (header.bitsPerSample == 32) {
				UMiscUtils::Resample_f32((float*)pcmData.data(), (float*)outPcmBuf.data(), header.sampleRate, mRequiredFormat->nSamplesPerSec, inFrames, header.numChannels);
				pcmData = outPcmBuf;
			}
			else if (header.bitsPerSample == 16) {
				UMiscUtils::Resample_s16((int16_t*)pcmData.data(), (int16_t*)outPcmBuf.data(), header.sampleRate, mRequiredFormat->nSamplesPerSec, inFrames, header.numChannels);
				pcmData = outPcmBuf;
			}
			else {
				// TODO
			}
		}
	}
}

void CWinAudioPlayerDlg::OnTaskCompleted()
{
	std::cout << "FFmpeg task done!" << std::endl;
	if (!mCacheFile.IsEmpty()) {
		parseWaveFile((LPCTSTR)mCacheFile);

		DeleteFile(mCacheFile); // 清理缓存
		mCacheFile.Empty();
	}

	this->GetDlgItem(IDC_BUTTON_PLAY)->EnableWindow(TRUE);
}

/*
* https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatex
* Specifies the number of bits per sample for the format type specified by wFormatTag.
If wFormatTag = WAVE_FORMAT_PCM, then wBitsPerSample should be set to either 8 or 16.
If wFormatTag = WAVE_FORMAT_IEEE_FLOAT, wBitsPerSample should be set to 32.

RIFF file format: https://github.com/audionamix/wave
*/
void WriteWaveFileHeader(std::fstream& file, UINT32 sampleRate, UINT16 numChannels, UINT16 bitsPerSample, UINT32 pcmDataSize)
{
	// WAVE文件头
	WAVEFORMATEX waveFormat;
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = numChannels;
	waveFormat.nSamplesPerSec = sampleRate;
	waveFormat.nAvgBytesPerSec = sampleRate * numChannels * (bitsPerSample / 8);
	waveFormat.nBlockAlign = numChannels * (bitsPerSample / 8);
	waveFormat.wBitsPerSample = bitsPerSample;
	waveFormat.cbSize = 0;

	// 创建数据块头
	DWORD chunkSize = 36 + pcmDataSize;
	DWORD fmtSize = sizeof(WAVEFORMATEX);

	// 写入RIFF标识和文件大小    
	file.write("RIFF", 4);
	file.write((const char*)&chunkSize, 4);

	// 写入WAVE标识    
	file.write("WAVE", 4);

	// 写入fmt子块    
	file.write("fmt ", 4);
	file.write((const char*)&fmtSize, 4);
	file.write((const char*)&waveFormat, sizeof(WAVEFORMATEX));

	// 写入data子块    
	file.write("data", 4);
	file.write((const char*)&pcmDataSize, 4);

	// 后面是PCM数据...
}

void CWinAudioPlayerDlg::OnBnClickedButtonPlay()
{
	UpdateData(TRUE);

	if (mAudioPlayer.IsPlaying()) {
		//AfxMessageBox(_T("The playback is still in progress..."));
		mAudioPlayer.Resume();
		return;
	}	

	PrepareForPlayback();
	mAudioPlayer.Start();

#ifdef _DUMP_PCM_FILE
	dumpFile.open(DUMP_FILENAME, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	dumpBytes = 0;
	WriteWaveFileHeader(dumpFile, mRequiredFormat->nSamplesPerSec, mRequiredFormat->nChannels, mRequiredFormat->wBitsPerSample, 0);
#endif
}

void CWinAudioPlayerDlg::OnBnClickedButtonPause()
{
	mAudioPlayer.Pause();
}

void CWinAudioPlayerDlg::OnBnClickedButtonStop()
{
	mAudioPlayer.Stop();

#ifdef _DUMP_PCM_FILE
	dumpFile.flush();
	// 修正Wave文件头信息
	dumpFile.seekp(0, std::ios::beg);
	WriteWaveFileHeader(dumpFile, mRequiredFormat->nSamplesPerSec, mRequiredFormat->nChannels, mRequiredFormat->wBitsPerSample, dumpBytes);
	dumpFile.close();
#endif

	MA_Stop();
}


HRESULT CWinAudioPlayerDlg::SetFormat(WAVEFORMATEX* pFormat)
{
	mRequiredFormat = pFormat;
	std::cout << "Required audio format >> Channels: " << pFormat->nChannels << " >> Sample Rate: " << pFormat->nSamplesPerSec << " >> Bits Per Sample: " << pFormat->wBitsPerSample << std::endl;
	return S_OK;
}

void synthesizeBuffer(int frame_count, int sample_rate, int bits_per_sample, int channel_count, BYTE* buffer)
{
	static const double sineFrequency = 440.0; // 440 Hz
	static const double cycleLength = sample_rate / sineFrequency;
	static const double PI = 3.14151926535;
	static int64_t N = 0; // Global counter of generated samples

	int16_t* pBuf16 = (int16_t*)buffer;
	int32_t* pBuf32 = (int32_t*)buffer;

	for (int a = 0; a < frame_count; a++)
	{
		// Calculate the next value of the sine wave sample.
		double value = sin(N * 2 * PI / cycleLength);

		if (bits_per_sample == 16) {
			// Convert to 16-bit value
			int16_t v16bit = static_cast<int16_t>(value * 32767);

			// Write the value to the buffer. The buffer has two channels.
			// Copy the same value to both.
			*pBuf16 = v16bit;
			pBuf16++;
			if (channel_count == 2) {
				*pBuf16 = v16bit;
				pBuf16++;
			}
		}
		else if (bits_per_sample == 32) {
			// Convert to 32-bit value
			int32_t v32bit = static_cast<int32_t>(value * 0x7FFFFFFF);

			// Write the value to the buffer. The buffer has two channels.
			// Copy the same value to both.
			*pBuf32 = v32bit;
			pBuf32++;
			if (channel_count == 2) {
				*pBuf32 = v32bit;
				pBuf32++;
			}
		}
		else {
			std::cout << "Not supported! Bits_per_sample " << bits_per_sample << std::endl;
		}

		// Increment the global sample counter.
		N++;
	}
}

// The audio engine represents sample values internally as floating-point numbers.
// to facilitate digital audio processing, the audio engine might use a mix format that represents samples as floating-point values.
// https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-getmixformat
void synthesizeBufferFloat(int frame_count, int sample_rate, int bits_per_sample, int channel_count, BYTE* buffer)
{
	static const double PI = 3.14151926535;
	static const float sineFrequency_ = 440.0; // Hz
	static const double cycleLength_ = sample_rate / sineFrequency_;
	static double pos_ = 0.0;

	float* out = (float*)buffer;

	for (int frame = 0; frame < frame_count; ++frame) {
		float val = sin(2 * PI * pos_ / cycleLength_);

		for (int chan = 0; chan < channel_count; ++chan) {
			out[frame * channel_count + chan] = val;
		}

		pos_ += 1.0;
		//if (pos_ > cycleLength_)
		//	pos_ -= cycleLength_;
	}
}

static int64_t bytesUsed = 0; // bytes which have been played

bool CWinAudioPlayerDlg::FillBufferWithFileData(UINT32 frameCount, BYTE* pData)
{
	int8_t* pSourceData = reinterpret_cast<int8_t*>(pcmData.data()) + bytesUsed;
	int64_t bytesLeft = pcmData.size() - bytesUsed;
	int64_t bytesToRead = frameCount * header.numChannels * header.bitsPerSample / 8;

	int framesToRead = frameCount;
	if (bytesToRead <= bytesLeft) {
		bytesUsed += bytesToRead;
	}
	else {
		framesToRead = bytesLeft / (header.numChannels * header.bitsPerSample / 8);
		bytesUsed = pcmData.size();
	}
	
	float* out = (float*)pData;

	int samplesAlreadyRead = 0;
	float sampleValue = 0.0;
	for (int frame = 0; frame < framesToRead; ++frame) {
		for (int chan = 0; chan < mRequiredFormat->nChannels; ++chan) { // 如果源文件的声道数多于设备的声道数，则超出的声道内容不播放
			sampleValue = 0.0;
			if (header.bitsPerSample == 8) {     // PCM数据标准化
				int8_t* pReadPos = (int8_t*)pSourceData + samplesAlreadyRead;
				if (chan < header.numChannels) { // 如果设备的声道数多于源文件，则填充静音数据
					sampleValue = pReadPos[chan] * 1.0 / 0xFF;
				}
			}
			else if (header.bitsPerSample == 16) {
				int16_t* pReadPos = (int16_t*)pSourceData + samplesAlreadyRead;
				if (chan < header.numChannels) {
					sampleValue = pReadPos[chan] * 1.0 / 0x7FFF;
				}
			}
			else if (header.bitsPerSample == 32) {
				/*int32_t* pReadPos = (int32_t*)pSourceData + samplesAlreadyRead;
				if (chan < header.numChannels) {
					sampleValue = pReadPos[chan] * 1.0 / 0x7FFFFFFF;
				}*/
				float* pReadPos = (float*)pSourceData + samplesAlreadyRead;
				if (chan < header.numChannels) {
					sampleValue = pReadPos[chan];
				}
			}
			else {
				std::cout << "Not supported! Bits_per_sample " << header.bitsPerSample << std::endl;
			}

			out[frame * mRequiredFormat->nChannels + chan] = sampleValue;
		}

		samplesAlreadyRead += header.numChannels;
	}

	return framesToRead > 0;
}

void CWinAudioPlayerDlg::PrepareForPlayback()
{
	bytesUsed = 0;
}

HRESULT CWinAudioPlayerDlg::LoadData(UINT32 frameCount, BYTE* pData, DWORD* flags)
{
	*flags = 0;
	if (mSourceFile.IsEmpty() || mIsSynth) {
		synthesizeBufferFloat(frameCount, mRequiredFormat->nSamplesPerSec, mRequiredFormat->wBitsPerSample, mRequiredFormat->nChannels, pData);
	}
	else {
		int bytesToFill = frameCount * mRequiredFormat->nChannels * mRequiredFormat->wBitsPerSample / 8;
		memset(pData, 0, bytesToFill);

		if (!FillBufferWithFileData(frameCount, pData))
		{
			*flags = AUDCLNT_BUFFERFLAGS_SILENT;
		}
	}

#ifdef _DUMP_PCM_FILE
	int bytes = frameCount * mRequiredFormat->nChannels * mRequiredFormat->wBitsPerSample / 8;
	dumpFile.write(reinterpret_cast<const char*>(pData), bytes);
	dumpBytes += bytes;
#endif

	return S_OK;
}

HRESULT CWinAudioPlayerDlg::OnAudioDeviceChanged(LPCWSTR pwstrDeviceId)
{
	// 最好在UI线程里更新UI，所以使用一个自定义消息
	PostMessage(WM_AUDIO_DEVICE_CHANGED);
	return S_OK;
}

LRESULT CWinAudioPlayerDlg::OnAudioDeviceChangedMessage(WPARAM wParam, LPARAM lParam)
{
	mSoundCardList.ResetContent();

	// 枚举所有音频播放设备
	std::vector<std::wstring> soundCards = UMiscUtils::GetAllSoundDevices();
	for (std::wstring& item : soundCards) {
		mSoundCardList.AddString(item.c_str());
	}
	mSoundCardList.SelectString(-1, UMiscUtils::GetDefaultSoundDevice().c_str()); // Select the default device

	OnCbnSelchangeComboSoundCards(); // 播放器重新初始化

	return 0;
}

// 关联.wav扩展名到本程序：
// Windows 11: 控制面板 | 应用 | 默认应用，为文件类型或链接类型设置默认值，输入".wav"，在电脑上选择应用 -> 指定到本程序
void CWinAudioPlayerDlg::TryToPlayFromCommandline()
{
	if (__argc < 2) return;

	for (int i = 1; i < __argc; i++) {
#if _UNICODE
		CString arg = __wargv[i];
#else
		CString arg = __argv[i];
#endif //_UNICODE
		if (arg.Find(_T(".wav")) != -1) {
			mSourceFile = arg;
			UpdateData(FALSE);

			parseWaveFile((LPCTSTR)mSourceFile);
			OnBnClickedButtonPlay();
			break;
		}
	}
}

void CWinAudioPlayerDlg::OnCbnSelchangeComboSoundCards()
{
	int curIndex = mSoundCardList.GetCurSel();
	if (curIndex >= 0) {
		CString strCard;
		mSoundCardList.GetLBText(curIndex, strCard);

		mAudioPlayer.Stop();
		mAudioPlayer.Uninit();
		mAudioPlayer.Init((LPCTSTR)strCard);
	}
}

// A handy library to play audio files https://github.com/mackron/miniaudio
// 支持的音频格式：WAV、MP3、FLAC
//	NOTE: ma_engine_play_sound 不支持宽字符文件路径！
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	// In playback mode copy data to pOutput. 
	// In capture mode read data from pInput. 
	// In full-duplex mode, both pOutput and pInput will be valid and you can move data from pInput into pOutput. 
	// Never process more than frameCount frames.
	ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
	if (pDecoder == NULL) {
		return;
	}

	ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

	(void)pInput;
}

void CWinAudioPlayerDlg::OnBnClickedButtonPlayWithMiniaudio()
{
	MA_PlayFile(mSourceFile);
}

int CWinAudioPlayerDlg::MA_PlayFile(const TCHAR* filepath)
{
	MA_Stop();

	ma_result result;

	result = ma_decoder_init_file_w(filepath, NULL, &maDecoder);
	if (result != MA_SUCCESS) {
		std::cout << "Failed to open source file.\n" << std::endl;
		return -1;
	}

	ma_device_config deviceConfig;
	deviceConfig = ma_device_config_init(ma_device_type_playback);
	// [Reference] engine_advanced.c
	// How to enumerate all playback devices, and give users a chance to select one. 
	//deviceConfig.playback.pDeviceID = ;
	deviceConfig.playback.format = maDecoder.outputFormat;
	deviceConfig.playback.channels = maDecoder.outputChannels;
	deviceConfig.sampleRate = maDecoder.outputSampleRate;
	deviceConfig.dataCallback = data_callback;
	deviceConfig.pUserData = &maDecoder;

	if (ma_device_init(NULL, &deviceConfig, &maDevice) != MA_SUCCESS) {
		std::cout << "Failed to open playback device.\n" << std::endl;
		ma_decoder_uninit(&maDecoder);
		return -2;
	}

	// [Reference] simple_mixing.c
	//ma_event_init(&g_stopEvent);

	if (ma_device_start(&maDevice) != MA_SUCCESS) {
		std::cout << "Failed to start playback device.\n" << std::endl;
		ma_device_uninit(&maDevice);
		ma_decoder_uninit(&maDecoder);
		return -3;
	}

	//std::cout << "Waiting for playback to complete...\n" << std::endl;
	//ma_event_wait(&g_stopEvent);

	return 0;
}

void CWinAudioPlayerDlg::MA_Stop()
{
	if (maDevice.state.value == ma_device_state_started) { // is playing?
		ma_device_stop(&maDevice);
		ma_device_uninit(&maDevice);
		ma_decoder_uninit(&maDecoder);
	}
}

typedef struct
{
	mp3dec_t* mp3d;
	mp3dec_file_info_t* info;
	size_t allocated;
} frames_iterate_data;

static int frames_iterate_cb(void* user_data, const uint8_t* frame, int frame_size, int free_format_bytes, size_t buf_size, uint64_t offset, mp3dec_frame_info_t* info)
{
	(void)buf_size;
	(void)offset;
	(void)free_format_bytes;

	frames_iterate_data* d = (frames_iterate_data*)user_data;
	d->info->channels = info->channels;
	d->info->hz = info->hz;
	d->info->layer = info->layer;
	/*printf("%d %d %d\n", frame_size, (int)offset, info->channels);*/
	if ((d->allocated - d->info->samples * sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t))
	{
		if (!d->allocated)
			d->allocated = 1024 * 1024;
		else
			d->allocated *= 2;
		mp3d_sample_t* alloc_buf = (mp3d_sample_t*)realloc(d->info->buffer, d->allocated);
		if (!alloc_buf)
			return MP3D_E_MEMORY;
		d->info->buffer = alloc_buf;
	}
	int samples = mp3dec_decode_frame(d->mp3d, frame, frame_size, d->info->buffer + d->info->samples, info);
	if (samples)
	{
		d->info->samples += samples * info->channels;
	}
	return 0;
}

void CWinAudioPlayerDlg::DecodeMp3ToPcmBuffer()
{
	if (mSourceFile.IsEmpty()) return;

	// 将MP3文件读进内存
/*	std::ifstream file((LPCTSTR)mSourceFile, std::ios::binary);
	if (file.is_open()) {
		file.seekg(0, std::ios::end);
		int fileSize = (int)file.tellg();
		file.seekg(0, std::ios::beg);
		mp3Data.resize(static_cast<size_t>(fileSize));
		file.read(reinterpret_cast<char*>(mp3Data.data()), fileSize);
		file.close();
	}*/

	// 解码
	mp3dec_file_info_t info;
	memset(&info, 0, sizeof(info));
	mp3dec_t mp3d;
	mp3dec_init(&mp3d);

	frames_iterate_data d = { &mp3d, &info, 0 };
	int ret = mp3dec_iterate_w((LPCTSTR)mSourceFile, frames_iterate_cb, &d);

	// 保存解码出来的PCM数据
	header.numChannels = info.channels;
	header.sampleRate = info.hz;
	header.bitsPerSample = sizeof(mp3d_sample_t) * 8;
	std::cout << "Source audio format: " << header.numChannels << " channels " << header.sampleRate << " Hz " << header.bitsPerSample << " bits" << std::endl;

	int totalBytes = info.samples * sizeof(mp3d_sample_t);
	pcmData.resize(static_cast<size_t>(totalBytes));
	memcpy(pcmData.data(), info.buffer, totalBytes); // TODO: 优化上述解码回调函数，以省去一次内存拷贝

	if (info.buffer)
		free(info.buffer);
}

CEasyWavePlayer gEasyWavePlayer;
UINT selectedDeviceId = 0;
void CWinAudioPlayerDlg::OnBnClickedButtonPlayWithLegacyWaveout()
{
	if (gEasyWavePlayer.IsPlaying()) return;

	// 交替指定各个音频设备进行播放
	selectedDeviceId = selectedDeviceId % gEasyWavePlayer.GetDevCount();
	std::cout << "Trying to play the wave file with device ID " << selectedDeviceId << std::endl;

	//gEasyWavePlayer.CheckDevCaps();

	gEasyWavePlayer.Play((LPCTSTR)mSourceFile, selectedDeviceId);

	selectedDeviceId++;
}
