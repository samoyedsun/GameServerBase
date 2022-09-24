#include "wstr2digit.h"
#include <sstream>

namespace wang
{
	template<typename T>
	void str2digit_dec(const char* buf, T& value)
	{
		stringstream ss;
		ss << buf;

		ss >> value;
	}

	template<>
	void str2digit_dec<int>(const char* buf, int& value)
	{

	}
}
