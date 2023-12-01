#include "pch.h"
#include "framework.h"
#include "UMiscUtils.h"

namespace UMiscUtils {

	// �����ַ��ַ���ת��Ϊ���ֽ��ַ���
	char* WtoA(const wchar_t* wstr)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* mbStr = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, mbStr, size, NULL, NULL);
		return mbStr;
	}

	// �����ֽ��ַ���ת��Ϊ���ַ��ַ���
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
			// ����Դ������Ŀ������ж�Ӧλ�õ�����
			double srcIndex = i / ratio;

			// ˫���Բ�ֵ����Ŀ��λ�õ�ȡֵ
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
		// �������Ƶ�ʱ���
		double ratio = static_cast<double>(dstFreq) / static_cast<double>(srcFreq);
		// ����Ŀ���������
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
			// TODO: �ݲ�֧�֣�
			resampledData = pcmData;
		}

		return resampledData;
	}
}