#pragma once

#include <thread>
#include <chrono>
#include <atomic>
#include <string>

class ITaskObserver {
public:
	virtual void OnTaskCompleted() = 0;
};

enum TASK_MODE
{
	FFMPEG,
	FFPLAY
};

class FFmpegTask 
{
public:
	FFmpegTask();
	~FFmpegTask();

	bool LocateTools(CString& toolFolder);
	void SetTaskObserver(ITaskObserver* observer) { m_StatusObserver = observer; }
	std::string Probe(CString& srcFile);

	void Run(CString& cmdParams);
	void Play(CString& cmdParams);
	bool IsRunning() { return m_bThreadRunning; }
	void Stop();

private:
	void DoRealTask();

private:
	std::thread m_thread;
	std::atomic<bool> m_bThreadRunning;
	ITaskObserver* m_StatusObserver;

	CString m_FFmpegFile;
	CString m_FFplayFile;
	CString m_FFprobeFile;
	TASK_MODE m_TaskMode;

	CString m_CmdParams;
};