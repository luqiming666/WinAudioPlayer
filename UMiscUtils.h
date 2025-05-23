#pragma once

#include <string>
#include <vector>

namespace UMiscUtils {

	CString GetRuntimeFilePath(TCHAR* filename = NULL, TCHAR* subFolder = NULL);
	CString GetProgramDataPath(TCHAR* subFolder, TCHAR* filename);
	std::string generateRandomFileName(const char* extension);

	char* WtoA(const wchar_t* wstr);
	wchar_t* AtoW(const char* str);

	std::vector<int8_t> Resample(std::vector<int8_t>& pcmData, int srcBitsPerSample, int srcFreq, int dstFreq);
	uint64_t Resample_f32(const float* input, float* output, int inSampleRate, int outSampleRate, uint64_t inputSize, uint32_t channels);
	uint64_t Resample_s16(const int16_t* input, int16_t* output, int inSampleRate, int outSampleRate, uint64_t inputSize, uint32_t channels);

	std::vector<std::wstring> GetAllSoundDevices();
	std::wstring GetDefaultSoundDevice();

	bool RunExternalApp(const TCHAR* appPath, TCHAR* appParams, bool bSync = false);
	bool RunExternalApp(TCHAR* fullCmd, std::string* outInfo, bool bSync = false);
}