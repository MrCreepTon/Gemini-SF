#include "EncodingUtil.h"

namespace EncodingUtil {
	std::string ConvertCP1251ToUTF8(const std::string& str) {
        int len = MultiByteToWideChar(1251, 0, str.c_str(), -1, NULL, 0);
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(1251, 0, str.c_str(), -1, wstr, len);

        len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, 0, 0);
        char* utf8 = new char[len];
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8, len, 0, 0);

        std::string result(utf8);
        delete[] wstr;
        delete[] utf8;

        return result;
	}

	std::string ConvertUTF8ToCP1251(const std::string& str) {
        int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, len);

        len = WideCharToMultiByte(1251, 0, wstr, -1, NULL, 0, NULL, NULL);
        char* cp1251 = new char[len];
        WideCharToMultiByte(1251, 0, wstr, -1, cp1251, len, NULL, NULL);

        std::string result(cp1251);
        delete[] wstr;
        delete[] cp1251;

        return result;
	}
}