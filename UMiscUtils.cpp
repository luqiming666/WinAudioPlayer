#include "pch.h"
#include "framework.h"
#include "UMiscUtils.h"
#include "Defs.h"
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <sstream>
#include <ctime>
#include <iomanip>

namespace UMiscUtils {

	CString GetRuntimeFilePath(TCHAR* filename, TCHAR* subFolder)
	{
		TCHAR szPath[MAX_PATH] = { 0 };
		::GetModuleFileName(NULL, szPath, MAX_PATH);

		CString strProcessPath(szPath);

		// ɾ���ļ������֣��õ���ͬĿ¼��·��
		PathRemoveFileSpec(strProcessPath.GetBuffer());
		strProcessPath.ReleaseBuffer();

		// ƴ��Ŀ���ļ�������·��
		if (subFolder) {
			TCHAR strTemp[MAX_PATH] = { 0 };
			PathCombine(strTemp, strProcessPath, subFolder);
			strProcessPath = strTemp;
		}

		if (filename) {
			CString strFinalFilePath;
			PathCombine(strFinalFilePath.GetBuffer(MAX_PATH), strProcessPath, filename);
			strFinalFilePath.ReleaseBuffer();

			return strFinalFilePath;
		}
		else {
			return strProcessPath;
		}
	}

	CString GetProgramDataPath(TCHAR* subFolder, TCHAR* filename)
	{
		TCHAR programDataPath[MAX_PATH];
		if (SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, programDataPath) != S_OK) {
			return _T("");
		}

		CString fullPath;
		PathCombine(fullPath.GetBuffer(MAX_PATH), programDataPath, subFolder);
		fullPath.ReleaseBuffer();
		if (!PathFileExists(fullPath)) {
			CreateDirectory(fullPath, NULL);
		}

		fullPath += _T("\\"); // ���·���ָ���
		fullPath += filename;
		return fullPath;
	}

	std::string formatTime() 
	{
		std::time_t rawtime = std::time(nullptr);
		struct tm timeinfo;
		localtime_s(&timeinfo, &rawtime);
		char buffer[20];
		sprintf_s(buffer, sizeof(buffer), "%04d%02d%02d%02d%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min);
		return std::string(buffer);
	}

	std::string generateRandomFileName() 
	{
		// ��ʼ�������������
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		std::stringstream ss;
		// ��������ַ�������
		for (int i = 0; i < 8; ++i) {
			char randomChar = static_cast<char>('a' + std::rand() % 26);
			ss << randomChar;
		}
		// ���ʱ�������
		std::string currentTime = formatTime();
		ss << currentTime;
		return ss.str();
	}

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

	// https://github.com/cpuimage/resampler
	uint64_t Resample_f32(const float* input, float* output, int inSampleRate, int outSampleRate, uint64_t inputSize,
		uint32_t channels
	) {
		if (input == NULL)
			return 0;
		uint64_t outputSize = (uint64_t)(inputSize * (double)outSampleRate / (double)inSampleRate);
		outputSize -= outputSize % channels;
		if (output == NULL)
			return outputSize;
		double stepDist = ((double)inSampleRate / (double)outSampleRate);
		const uint64_t fixedFraction = (1LL << 32);
		const double normFixed = (1.0 / (1LL << 32));
		uint64_t step = ((uint64_t)(stepDist * fixedFraction + 0.5));
		uint64_t curOffset = 0;
		for (uint32_t i = 0; i < outputSize; i++) {
			for (uint32_t c = 0; c < channels; c++) {
				*output++ = (float)(input[c] + (input[c + channels] - input[c]) * (
					(double)(curOffset >> 32) + ((curOffset & (fixedFraction - 1)) * normFixed)
					)
					);
			}
			curOffset += step;
			input += (curOffset >> 32) * channels;
			curOffset &= (fixedFraction - 1);
		}
		return outputSize;
	}

	uint64_t Resample_s16(const int16_t* input, int16_t* output, int inSampleRate, int outSampleRate, uint64_t inputSize,
		uint32_t channels
	) {
		if (input == NULL)
			return 0;
		uint64_t outputSize = (uint64_t)(inputSize * (double)outSampleRate / (double)inSampleRate);
		outputSize -= outputSize % channels;
		if (output == NULL)
			return outputSize;
		double stepDist = ((double)inSampleRate / (double)outSampleRate);
		const uint64_t fixedFraction = (1LL << 32);
		const double normFixed = (1.0 / (1LL << 32));
		uint64_t step = ((uint64_t)(stepDist * fixedFraction + 0.5));
		uint64_t curOffset = 0;
		for (uint32_t i = 0; i < outputSize; i++) {
			for (uint32_t c = 0; c < channels; c++) {
				*output++ = (int16_t)(input[c] + (input[c + channels] - input[c]) * (
					(double)(curOffset >> 32) + ((curOffset & (fixedFraction - 1)) * normFixed)
					)
					);
			}
			curOffset += step;
			input += (curOffset >> 32) * channels;
			curOffset &= (fixedFraction - 1);
		}
		return outputSize;
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

	bool RunExternalApp(TCHAR* appPath, TCHAR* appParams, bool bSync)
	{
		// ����������Ϣ�ṹ��
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(processInfo));

		// ����������Ϣ�ṹ��
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		// �����ⲿ���
		BOOL bSuccess = CreateProcess(
			appPath,					// �ⲿ�����·��
			appParams,					// �����в���
			NULL,                       // Ĭ�ϰ�ȫ��������
			NULL,                       // Ĭ�ϰ�ȫ��������
			FALSE,                      // ���̳о��
			CREATE_NO_WINDOW,           // ��Ϊ�½��̴���CUI����
			NULL,                       // Ĭ�ϻ�������
			NULL,                       // Ĭ�Ϲ���Ŀ¼
			&startupInfo,               // ������Ϣ�ṹ��
			&processInfo                // ������Ϣ�ṹ��
		);

		if (bSuccess && bSync)
		{
			// �ȴ������˳�
			WaitForSingleObject(processInfo.hProcess, INFINITE);
		}

		// �رս��̺��̵߳ľ��
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return bSuccess == TRUE;
	}

	bool RunExternalApp(TCHAR* fullCmd, std::string* outInfo, bool bSync)
	{
		// ����������Ϣ�ṹ��
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(processInfo));

		// ����������Ϣ�ṹ��
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		// �Ƿ���Ҫ��ȡ����Ŀ���̨������ݣ�
		HANDLE hReadPipe = NULL;
		HANDLE hWritePipe = NULL;
		BOOL bPipeCreated = FALSE;
		if (outInfo) {
			SECURITY_ATTRIBUTES saAttr;
			saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
			saAttr.bInheritHandle = TRUE;
			saAttr.lpSecurityDescriptor = NULL;
			bPipeCreated = CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0);

			startupInfo.hStdError = hWritePipe;
			startupInfo.hStdOutput = hWritePipe;
			startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		}

		//if (!visible) {
			//startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			//startupInfo.wShowWindow = SW_HIDE; // ���������д���
		//}		

		// �����ⲿ���
		BOOL bSuccess = CreateProcess(
			NULL,						// �ⲿ�����·��
			fullCmd,					// �����в���
			NULL,                       // Ĭ�ϰ�ȫ��������
			NULL,                       // Ĭ�ϰ�ȫ��������
			TRUE,						// �̳о��!!!
			CREATE_NO_WINDOW,           // ��Ϊ�½��̴���CUI����
			NULL,                       // Ĭ�ϻ�������
			NULL,                       // Ĭ�Ϲ���Ŀ¼
			&startupInfo,               // ������Ϣ�ṹ��
			&processInfo                // ������Ϣ�ṹ��
		);

		if (bSuccess && bSync)
		{
			// �ȴ������˳�
			WaitForSingleObject(processInfo.hProcess, 1000);
		}

		// ��ȡ�ܵ�����
		if (bPipeCreated) {
			CloseHandle(hWritePipe);

			DWORD dwRead;
			CHAR chBuf[4096];
			BOOL success;
			while (true) {
				success = ReadFile(hReadPipe, chBuf, sizeof(chBuf), &dwRead, NULL);
				if (!success || dwRead == 0) break;
				std::string s(chBuf, dwRead);
				*outInfo += s;
			}
			CloseHandle(hReadPipe);
		}

		// �رս��̺��̵߳ľ��
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return bSuccess == TRUE;
	}
}