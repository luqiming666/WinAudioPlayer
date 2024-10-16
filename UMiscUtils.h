#pragma once

#include <string>
#include <vector>
#include <list>

namespace UMiscUtils {

	char* WtoA(const wchar_t* wstr);
	wchar_t* AtoW(const char* str);

	std::vector<int8_t> Resample(std::vector<int8_t>& pcmData, int srcBitsPerSample, int srcFreq, int dstFreq);
	uint64_t Resample_f32(const float* input, float* output, int inSampleRate, int outSampleRate, uint64_t inputSize, uint32_t channels);
	uint64_t Resample_s16(const int16_t* input, int16_t* output, int inSampleRate, int outSampleRate, uint64_t inputSize, uint32_t channels);

	std::list<std::wstring> GetAllSoundCards();
	std::wstring GetDefaultSoundCard();
}