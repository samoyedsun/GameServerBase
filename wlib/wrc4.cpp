#include "wrc4.h"
#include <string.h>

namespace wang
{
	wRC4::wRC4() : m_x(0), m_y(0)
	{
		memset(m_data, 0, sizeof(m_data));
	}

	void wRC4::set_key(int len, const unsigned char* data)
	{
		//init
		for (int i = 0; i < 256; ++i)
		{
			m_data[i] = i;
		}

		//range
		int j = 0;
		unsigned char tmp;
		for (int i = 0; i < 256; ++i)
		{
			j = (j + m_data[i] + data[i % len]) & 0xff;
			tmp = m_data[i];
			m_data[i] = m_data[j];
			m_data[j] = tmp;
		}

		m_x = 0;
		m_y = 0;
	}

	void wRC4::process(int len, const unsigned char* in_data, unsigned char* out_data)
	{
		int i = m_x, j = m_y, t = 0;
		unsigned char tmp;
		for (int r = 0; r < len; ++r)
		{
			i = (i + 1) & 0xff;
			j = (j + m_data[i]) & 0xff;

			tmp = m_data[i];
			m_data[i] = m_data[j];
			m_data[j] = tmp;

			t = (m_data[i] + m_data[j]) & 0xff;

			out_data[r] = static_cast<unsigned char>(in_data[r] ^ m_data[t]);
		}
		m_x = i;
		m_y = j;
	}
}
