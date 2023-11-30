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
}