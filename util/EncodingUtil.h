#pragma once

#include "main.h"

namespace EncodingUtil {
	std::string ConvertCP1251ToUTF8(const std::string& str);
	std::string ConvertUTF8ToCP1251(const std::string& str);
}