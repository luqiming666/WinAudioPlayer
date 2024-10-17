#include "pch.h"
#include "framework.h"
#include "FFmpegTask.h"
#include "UMiscUtils.h"

FFmpegTask::FFmpegTask()
	: m_bThreadRunning(false)
    , m_StatusObserver(NULL)
    , m_TaskMode(FFMPEG)
{
}

FFmpegTask::~FFmpegTask()
{
}

bool FFmpegTask::LocateTools(CString& toolFolder)
{
    TCHAR strTemp[MAX_PATH] = { 0 };
    PathCombine(strTemp, toolFolder, _T("ffmpeg.exe"));
    m_FFmpegFile = strTemp;

    PathCombine(strTemp, toolFolder, _T("ffplay.exe"));
    m_FFplayFile = strTemp;

    PathCombine(strTemp, toolFolder, _T("ffprobe.exe"));
    m_FFprobeFile = strTemp;
    
    return PathFileExists(m_FFmpegFile);
}

std::string FFmpegTask::Probe(CString& srcFile)
{
    CString strFullCmd;
    strFullCmd.Format(_T("%s -v quiet -print_format json -show_format -show_streams %s"), (LPCTSTR)m_FFprobeFile, (LPCTSTR)srcFile);

    std::string result = "";
    UMiscUtils::RunExternalApp(strFullCmd.GetBuffer(), &result, false);
    strFullCmd.ReleaseBuffer();

    /*char buffer[4096];
    FILE* pipe = _tpopen(strCmd, _T("r"));
    if (pipe)
    {
        try {
            while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
                result += buffer;
            }
        }
        catch (...) {
            _pclose(pipe);
            throw;
        }
        _pclose(pipe);
    }*/
    return result;

}

void FFmpegTask::Run(CString& cmdParams)
{
    m_CmdParams = cmdParams;
    m_TaskMode = FFMPEG;

    m_bThreadRunning = true;
    m_thread = std::thread(&FFmpegTask::DoRealTask, this);
}

void FFmpegTask::Stop()
{
    m_bThreadRunning = false;
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void FFmpegTask::DoRealTask()
{
    while (m_bThreadRunning)
    {
        CString strToolPath = m_FFmpegFile;
        if (m_TaskMode == FFPLAY) {
            strToolPath = m_FFplayFile;
        }

        UMiscUtils::RunExternalApp((LPCTSTR)strToolPath, m_CmdParams.GetBuffer(), true);
        m_CmdParams.ReleaseBuffer();

        //std::this_thread::sleep_for(std::chrono::seconds(1));
        m_bThreadRunning = false;
    }

    if (m_StatusObserver) {
        m_StatusObserver->OnTaskCompleted();
    }
    m_thread.detach();
}

void FFmpegTask::Play(CString& cmdParams)
{
    m_CmdParams = cmdParams;
    m_TaskMode = FFPLAY;

    m_bThreadRunning = true;
    m_thread = std::thread(&FFmpegTask::DoRealTask, this);
}
