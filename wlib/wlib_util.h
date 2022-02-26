#ifndef W_UTIL_H
#define W_UTIL_H

#include "wnet_type.h"
#include <string>

namespace wang
{
	extern const std::string g_null_str;
	void wsleep(uint32 milliseconds);

	int count_1(uint32 x);
}

#endif
