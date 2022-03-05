#include "wrc4.h"

namespace wang
{
	wRC4::wRC4()
	{
	}

	void wRC4::set_key(int len, const unsigned char* data)
	{
		int i = 0, j = 0;
		char k[256] = { 0 };
		unsigned char tmp = 0;
		for (i = 0; i<256; i++) {
			m_data[i] = i;
			k[i] = data[i%len];
		}
		for (i = 0; i<256; i++) {
			j = (j + m_data[i] + k[i]) % 256;
			tmp = m_data[i];
			m_data[i] = m_data[j];
			m_data[j] = tmp;
		}
	}

	void wRC4::process(int len, const unsigned char* in_data, unsigned char* out_data)
	{
		int i = 0, j = 0, t = 0;
		unsigned long k = 0;
		unsigned char tmp;
		for (k = 0; k<len; k++) {
			i = (i + 1) % 256;
			j = (j + m_data[i]) % 256;
			tmp = m_data[i];
			m_data[i] = m_data[j];
			m_data[j] = tmp;
			t = (m_data[i] + m_data[j]) % 256;
			out_data[k] = in_data[k] ^ m_data[t];
		}
	}
}