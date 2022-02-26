#include "wlib_util.h"
#include <thread>
#include <chrono>

namespace wang {

	const std::string g_null_str;

	void wsleep(uint32 ms)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}

	int count_1(uint32 x)
	{
		int countx = 0;
		while (x)
		{
			countx++;
			x = x & (x - 1);
		}
		return countx;
	}
} // namespace wang
