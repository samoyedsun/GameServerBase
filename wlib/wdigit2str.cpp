#include "wdigit2str.h"
#include <algorithm>
#include <cstdio>

#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif

namespace wang
{
	static const char s_dec_digits[] = "9876543210123456789";
	static const char* s_dec_zero = s_dec_digits + 9;

	// Efficient Integer to String Conversions, by Matthew Wilson.
	template<typename T>
	int digit2str_dec(char* buf, int buf_size, T value)
	{
		T i = value;
		char* p = buf;

		do
		{
			int lsd = static_cast<int>(i % 10);
			i /= 10;
			if (p >= p + buf_size)
				return 0;
			*p++ = s_dec_zero[lsd];
		} while (i != 0);

		if (value < 0)
		{
			if (p >= p + buf_size)
				return 0;
			*p++ = '-';
		}
		*p = '\0';
		std::reverse(buf, p);

		return static_cast<int>(p - buf);
	}

	template<>
	int digit2str_dec<float>(char* buf, int buf_size, float value)
	{
		int len = sprintf(buf, "%.2g", value);
		if (len >= buf_size)
			return 0;
		*(buf + len) = '\0';
		return len;
	}

	template<>
	int digit2str_dec<double>(char* buf, int buf_size, double value)
	{
		int len = sprintf(buf, "%.2g", value);
		if (len >= buf_size)
			return 0;
		*(buf + len) = '\0';
		return len;
	}

	//Explicit Instantiation

#define IMPL1(TYPE) template int digit2str_dec<TYPE>(char*, int, TYPE);

#define IMPL2(TYPE) IMPL1(signed TYPE) IMPL1(unsigned TYPE)

		IMPL1(char)
		IMPL2(char)
		IMPL2(short)
		IMPL2(int)
		IMPL2(long)
		IMPL2(long long)

#undef IMPL1
#undef IMPL2
}

namespace wang
{
	static const char s_hex_digits[] = "0123456789ABCDEF";

	template<typename T>
	int int2str_hex(char* buf, T value)
	{
		T i = value;
		char* p = buf;

		do
		{
			int lsd = static_cast<int>(i % 16);
			i /= 16;
			*p++ = s_hex_digits[lsd];
		} while (i != 0);

		*p = '\0';
		std::reverse(buf, p);

		return static_cast<int>(p - buf);
	}

#define IMPL1(TYPE) template int int2str_hex<TYPE>(char*, TYPE);

#define IMPL2(TYPE) IMPL1(unsigned TYPE)\
	template<>\
	int int2str_hex<signed TYPE>(char* buf, signed TYPE value)\
	{\
		return int2str_hex<unsigned TYPE>(buf, value);\
	}

	IMPL1(char)
		IMPL2(char)
		IMPL2(short)
		IMPL2(int)
		IMPL2(long)
		IMPL2(long long)

#undef IMPL1
#undef IMPL2
}

namespace wang
{
	void byte2hex(char* dst_buf, const unsigned char *src_buf, int buf_len)
	{
		for (int i = 0; i < buf_len; ++i)
		{
			*dst_buf++ = s_hex_digits[src_buf[i] / 16];
			*dst_buf++ = s_hex_digits[src_buf[i] % 16];
		}
		*dst_buf = '\0';
	}
}