#pragma once

#include <Windows.h>
#include <string>

using namespace std;

class MyUtility
{
public:
	static string MultibyteToUtf8(const string& str);

	static string Utf8ToMultibyte(const string& str);

};

