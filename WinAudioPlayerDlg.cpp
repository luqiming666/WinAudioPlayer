﻿
// WinAudioPlayerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "WinAudioPlayer.h"
#include "WinAudioPlayerDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 打开一个控制台窗口，方便查看调试信息
#define DEBUG_WINDOW_IS_OPEN 1

#define _DUMP_PCM_FILE

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
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWinAudioPlayerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SOURCE_FILE, mSourceFile);
}

BEGIN_MESSAGE_MAP(CWinAudioPlayerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_BROWSER, &CWinAudioPlayerDlg::OnBnClickedButtonBrowser)
	ON_BN_CLICKED(IDC_BUTTON_PLAY, &CWinAudioPlayerDlg::OnBnClickedButtonPlay)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CWinAudioPlayerDlg::OnBnClickedButtonStop)
	ON_WM_DESTROY()
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

	// 初始化COM组件
	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	mAudioPlayer.Init(this);

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

	// 反初始化COM组件
	CoUninitialize();
}


void CWinAudioPlayerDlg::OnBnClickedButtonBrowser()
{
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Wave Files (*.wav)|*.wav|All Files (*.*)|*.*||"), NULL);
	if (fileDlg.DoModal() == IDOK)
	{
		mSourceFile = fileDlg.GetPathName();
		UpdateData(FALSE);
	}
}


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
	mAudioPlayer.Start();

#ifdef _DUMP_PCM_FILE
	dumpFile.open(DUMP_FILENAME, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
	dumpBytes = 0;
	WriteWaveFileHeader(dumpFile, mRequiredFormat->nSamplesPerSec, mRequiredFormat->nChannels, mRequiredFormat->wBitsPerSample, 0);
#endif
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
}


void CWinAudioPlayerDlg::SetFormat(WAVEFORMATEX* pFormat)
{
	mRequiredFormat = pFormat;
	std::cout << "Audio format >> Channels: " << pFormat->nChannels << " Sample-rate: " << pFormat->nSamplesPerSec << " Bit-depth: " << pFormat->wBitsPerSample << std::endl;
}

// Global counter of generated samples
int64_t N = 0;
void synthesizeBuffer(int frame_count, int sample_rate, int bits_per_sample, int channel_count, BYTE* buffer)
{
	double A440 = 440.0; // 440 Hz
	double PI = 3.14151926535;

	int16_t* pBuf16 = (int16_t*)buffer;
	int32_t* pBuf32 = (int32_t*)buffer;

	for (int a = 0; a < frame_count; a++)
	{
		// Calculate the next value of the sine wave sample.
		double value = 0.1 * sin(N * A440 * 2 * PI / (double)sample_rate);

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

HRESULT CWinAudioPlayerDlg::LoadData(UINT32 frameCount, BYTE* pData, DWORD* flags)
{
	synthesizeBuffer(frameCount, mRequiredFormat->nSamplesPerSec, mRequiredFormat->wBitsPerSample, mRequiredFormat->nChannels, pData);
	*flags = 0;
	//*flags = AUDCLNT_BUFFERFLAGS_SILENT;

#ifdef _DUMP_PCM_FILE
	int bytes = frameCount * mRequiredFormat->nChannels * mRequiredFormat->wBitsPerSample / 8;
	dumpFile.write(reinterpret_cast<const char*>(pData), bytes);
	dumpBytes += bytes;
#endif

	return S_OK;
}