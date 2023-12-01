#include "pch.h"
#include "framework.h"
#include "UMiscUtils.h"

namespace UMiscUtils {

	// 将宽字符字符串转换为多字节字符串
	char* WtoA(const wchar_t* wstr)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* mbStr = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, mbStr, size, NULL, NULL);
		return mbStr;
	}

	// 将多字节字符串转换为宽字符字符串
	wchar_t* AtoW(const char* str)
	{
		int size = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
		wchar_t* wcStr = new wchar_t[size];
		MultiByteToWideChar(CP_UTF8, 0, str, -1, wcStr, size);
		return wcStr;
	}

	template <class T>
	void _internalResample(T* pSrcData, T* pDstData, int srcSampleCount, int dstSampleCount, double ratio) {
		for (int i = 0; i < dstSampleCount; ++i) {
			// 计算源数据在目标采样中对应位置的索引
			double srcIndex = i / ratio;

			// 双线性插值计算目标位置的取值
			int floorIndex = static_cast<int>(srcIndex);
			int ceilIndex = floorIndex + 1;
			if (ceilIndex >= srcSampleCount) {
				ceilIndex = floorIndex;
			}
			double fraction = srcIndex - floorIndex;

			T floorValue = pSrcData[floorIndex];
			T ceilValue = pSrcData[ceilIndex];
			T interpolatedValue = static_cast<T>((1.0 - fraction) * floorValue + fraction * ceilValue);
			pDstData[i] = interpolatedValue;
		}
	}

	// https://linuxcpp.0voice.com/?id=126766
	std::vector<int8_t> Resample(std::vector<int8_t>& pcmData, int srcBitsPerSample, int srcFreq, int dstFreq)
	{
		std::vector<int8_t> resampledData;
		// 计算采样频率比例
		double ratio = static_cast<double>(dstFreq) / static_cast<double>(srcFreq);
		// 计算目标采样点数
		int dstSize = static_cast<int>(pcmData.size() * ratio);
		resampledData.resize(dstSize);

		int bytesPerSample = srcBitsPerSample / 8;
		int dstSampleCount = dstSize / bytesPerSample;
		int srcSampleCount = pcmData.size() / bytesPerSample;

		if (srcBitsPerSample == 8) {
			int8_t* pSrcData = reinterpret_cast<int8_t*>(pcmData.data());
			int8_t* pDstData = reinterpret_cast<int8_t*>(resampledData.data());
			_internalResample(pSrcData, pDstData, srcSampleCount, dstSampleCount, ratio);
		}
		else if (srcBitsPerSample == 16) {
			int16_t* pSrcData = reinterpret_cast<int16_t*>(pcmData.data());
			int16_t* pDstData = reinterpret_cast<int16_t*>(resampledData.data());
			_internalResample(pSrcData, pDstData, srcSampleCount, dstSampleCount, ratio);
		}
		else if (srcBitsPerSample == 32) {
			int32_t* pSrcData = reinterpret_cast<int32_t*>(pcmData.data());
			int32_t* pDstData = reinterpret_cast<int32_t*>(resampledData.data());
			_internalResample(pSrcData, pDstData, srcSampleCount, dstSampleCount, ratio);
		}
		else {
			// TODO: 暂不支持！
			resampledData = pcmData;
		}

		return resampledData;
	}
}