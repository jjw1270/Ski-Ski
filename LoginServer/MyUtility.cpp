#include "MyUtility.h"

string MyUtility::MultibyteToUtf8(const string& str) {
	int nLen = str.size();
	wchar_t warr[1024];
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), -1, warr, 1024);
	char carr[1024];
	memset(carr, '\0', sizeof(carr));
	WideCharToMultiByte(CP_UTF8, 0, warr, -1, carr, 1024, NULL, NULL);
	return carr;
}

string MyUtility::Utf8ToMultibyte(const string& str) {
	wchar_t warr[1024];
	int nLen = str.size();
	memset(warr, '\0', sizeof(warr));
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, warr, 1024);
	char carr[1024];
	memset(carr, '\0', sizeof(carr));
	WideCharToMultiByte(CP_ACP, 0, warr, -1, carr, 1024, NULL, NULL);
	return carr;
}
