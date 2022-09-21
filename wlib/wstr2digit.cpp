#include "wdigit2str.h"
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
}
