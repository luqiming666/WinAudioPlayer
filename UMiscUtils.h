#pragma once

#include <vector>

namespace UMiscUtils {

	char* WtoA(const wchar_t* wstr);
	wchar_t* AtoW(const char* str);

	std::vector<int8_t> Resample(std::vector<int8_t>& pcmData, int srcBitsPerSample, int srcFreq, int dstFreq);
}