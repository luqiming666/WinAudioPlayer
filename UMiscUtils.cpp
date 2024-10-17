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

		// 删除文件名部分，得到相同目录的路径
		PathRemoveFileSpec(strProcessPath.GetBuffer());
		strProcessPath.ReleaseBuffer();

		// 拼接目标文件的完整路径
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

		fullPath += _T("\\"); // 添加路径分隔符
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
		// 初始化随机数生成器
		std::srand(static_cast<unsigned int>(std::time(nullptr)));
		std::stringstream ss;
		// 生成随机字符串部分
		for (int i = 0; i < 8; ++i) {
			char randomChar = static_cast<char>('a' + std::rand() % 26);
			ss << randomChar;
		}
		// 添加时间戳部分
		std::string currentTime = formatTime();
		ss << currentTime;
		return ss.str();
	}

	// 将宽字符字符串转换为多字节字符串
	//	记得释放内存！
	char* WtoA(const wchar_t* wstr)
	{
		int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
		char* mbStr = new char[size];
		WideCharToMultiByte(CP_UTF8, 0, wstr, -1, mbStr, size, NULL, NULL);
		return mbStr;
	}

	// 将多字节字符串转换为宽字符字符串
	//	记得释放内存！
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
		// 创建进程信息结构体
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(processInfo));

		// 创建启动信息结构体
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		// 启动外部软件
		BOOL bSuccess = CreateProcess(
			appPath,					// 外部软件的路径
			appParams,					// 命令行参数
			NULL,                       // 默认安全性描述符
			NULL,                       // 默认安全性描述符
			FALSE,                      // 不继承句柄
			CREATE_NO_WINDOW,           // 不为新进程创建CUI窗口
			NULL,                       // 默认环境变量
			NULL,                       // 默认工作目录
			&startupInfo,               // 启动信息结构体
			&processInfo                // 进程信息结构体
		);

		if (bSuccess && bSync)
		{
			// 等待进程退出
			WaitForSingleObject(processInfo.hProcess, INFINITE);
		}

		// 关闭进程和线程的句柄
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return bSuccess == TRUE;
	}

	bool RunExternalApp(TCHAR* fullCmd, std::string* outInfo, bool bSync)
	{
		// 创建进程信息结构体
		PROCESS_INFORMATION processInfo;
		ZeroMemory(&processInfo, sizeof(processInfo));

		// 创建启动信息结构体
		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(startupInfo));
		startupInfo.cb = sizeof(startupInfo);

		// 是否需要获取程序的控制台输出内容？
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
			//startupInfo.wShowWindow = SW_HIDE; // 隐藏命令行窗口
		//}		

		// 启动外部软件
		BOOL bSuccess = CreateProcess(
			NULL,						// 外部软件的路径
			fullCmd,					// 命令行参数
			NULL,                       // 默认安全性描述符
			NULL,                       // 默认安全性描述符
			TRUE,						// 继承句柄!!!
			CREATE_NO_WINDOW,           // 不为新进程创建CUI窗口
			NULL,                       // 默认环境变量
			NULL,                       // 默认工作目录
			&startupInfo,               // 启动信息结构体
			&processInfo                // 进程信息结构体
		);

		if (bSuccess && bSync)
		{
			// 等待进程退出
			WaitForSingleObject(processInfo.hProcess, 1000);
		}

		// 读取管道内容
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

		// 关闭进程和线程的句柄
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return bSuccess == TRUE;
	}
}