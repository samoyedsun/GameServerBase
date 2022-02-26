#include "wrandom.h"
#include <ctime>

namespace wang {

	
	wrandom::wrandom()
	{
		init();
	}

	wrandom::wrandom(uint32 seed)
	{
		init(seed);
	}

	void wrandom::init()
	{
		init((uint32)time(NULL));
	}

	void wrandom::init(uint32 seed)
	{
		uint32 i;
		m_MT[0] = seed;
		for (i = 1; i < MAX_SEED_NUM; i++)
		{
			m_MT[i] = (1812433253 * (m_MT[i - 1] ^ (m_MT[i - 1] >> 30)) + i);
		}
		m_index = MAX_SEED_NUM;
	}

	uint32 wrandom::rand()
	{
		uint32 y;
		uint32 i = m_index;
		if (m_index >= MAX_SEED_NUM)
		{
			uint32 j, x, xA;
			for (j = 0; j < MAX_SEED_NUM; j++)
			{
				x = (m_MT[j] & 0x80000000) + (m_MT[(j + 1) % MAX_SEED_NUM] & 0x7fffffff);
				xA = x >> 1;
				if (x & 0x1)
					xA ^= 0x9908B0DF;
				m_MT[j] = m_MT[(j + 397) % MAX_SEED_NUM] ^ xA;
			}
			m_index = 0;
			i = m_index;
		}
		y = m_MT[i];
		m_index = i + 1;
		y ^= (y >> 11);
		y ^= (y << 7) & 0x9D2C5680;
		y ^= (y << 15) & 0xEFC60000;
		y ^= (y >> 18);
		return y;
	}

	float wrandom::randf()
	{
		return s_rand.rand() / double(0xffffffff);
	}

	uint32 wrandom::rand(uint32 minValue, uint32 maxValue)
	{
		return rand() % (maxValue - minValue + 1) + minValue;
	}

	uint32 wrandom::rand(uint32 maxValue)
	{
		return rand() % maxValue;
	}

}