#include "pch.h"
#include "framework.h"
#include "UMiscUtils.h"
#include "Defs.h"
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>

namespace UMiscUtils {

	// �����ַ��ַ���ת��Ϊ���ֽ��ַ���
	//	�ǵ��ͷ��ڴ棡
	char* WtoA(const wchar_t* wstr)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* mbStr = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, mbStr, size, NULL, NULL);
		return mbStr;
	}

	// �����ֽ��ַ���ת��Ϊ���ַ��ַ���
	//	�ǵ��ͷ��ڴ棡
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

	std::list<std::wstring> GetAllSoundCards()
	{
		std::list<std::wstring> allCards;

		IMMDeviceEnumerator* pEnumerator = NULL;
		IMMDeviceCollection* pDevices = NULL;

		HRESULT hr;
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
		EXIT_ON_ERROR(hr);

		hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
		EXIT_ON_ERROR(hr);

		UINT deviceCount = 0;
		hr = pDevices->GetCount(&deviceCount);
		if (deviceCount > 0) {
			for (int i = 0; i < deviceCount; i++) {
				IMMDevice* pDeviceItem = NULL;
				IPropertyStore* pProps = NULL;
				if (SUCCEEDED(pDevices->Item(i, &pDeviceItem))) {
					hr = pDeviceItem->OpenPropertyStore(STGM_READ, &pProps);

					PROPVARIANT varName;
					PropVariantInit(&varName);
					hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);    
					if (varName.vt != VT_EMPTY)
					{
						allCards.push_back(varName.pwszVal);
					}
					PropVariantClear(&varName);
				}
				SAFE_RELEASE(pProps);
				SAFE_RELEASE(pDeviceItem);
			}
		}

	Exit:
		SAFE_RELEASE(pDevices);
		SAFE_RELEASE(pEnumerator);
		return allCards;
	}

	std::wstring GetDefaultSoundCard()
	{
		IMMDeviceEnumerator* pEnumerator = NULL;
		IMMDevice* pDevice = NULL;
		IPropertyStore* pProps = NULL;
		std::wstring		deviceName;

		HRESULT hr;
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
		EXIT_ON_ERROR(hr);

		hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
		EXIT_ON_ERROR(hr);

		hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
		EXIT_ON_ERROR(hr);

		PROPVARIANT varName;
		PropVariantInit(&varName);
		hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
		if (varName.vt != VT_EMPTY)
		{
			deviceName = varName.pwszVal;
		}
		PropVariantClear(&varName);

	Exit:
		SAFE_RELEASE(pProps);
		SAFE_RELEASE(pDevice);
		SAFE_RELEASE(pEnumerator);
		return deviceName;
	}
}