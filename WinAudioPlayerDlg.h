
// WinAudioPlayerDlg.h: 头文件
//

#pragma once

#include "CAudioBufPlayer.h"

// CWinAudioPlayerDlg 对话框
class CWinAudioPlayerDlg : public CDialogEx, public IAudioSource
{
// 构造
public:
	CWinAudioPlayerDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WINAUDIOPLAYER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	CComboBox mSoundCardList;

	CAudioBufPlayer mAudioPlayer;
	WAVEFORMATEX* mRequiredFormat;
	CString mSourceFile;

	// IAudioSource methods
	HRESULT SetFormat(WAVEFORMATEX* pFormat);
	HRESULT LoadData(UINT32 frameCount, BYTE* pData, DWORD* flags);

	void PrepareForPlayback();
	bool FillBufferWithFileData(UINT32 frameCount, BYTE* pData);
	void TryToPlayFromCommandline();

	// Test MiniAudio APIs
	int MA_PlayFile(const TCHAR* filepath);
	void MA_Stop();

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonBrowser();
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnDestroy();

	afx_msg void OnCbnSelchangeComboSoundCards();
	afx_msg void OnBnClickedButtonPlayWithMiniaudio();
};
