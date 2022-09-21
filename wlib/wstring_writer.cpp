#include "wstring_writer.h"

namespace wang {

	wstring_writer::wstring_writer(char * buf, unsigned int size)
		: wbuffer(buf, size)
	{
	}

	wstring_writer& wstring_writer::operator<<(bool value)
	{
		if (value)
		{
			const char *v = "true";
			write(v, strlen(v));
		}
		else
		{
			const char *v = "false";
			write(v, strlen(v));
		}
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(char value)
	{
		write(&value, sizeof(value));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(signed char value)
	{
		write(&value, sizeof(value));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(unsigned char value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%u", (int)value);
#else
		snprintf(str, "%u", (int)value);
#endif
		write(str, strlen(str));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(signed short value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%d", (int)value);
#else
		snprintf(str, "%d", (int)value);
#endif
		write(str, strlen(str));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(unsigned short value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%u", (int)value);
#else
		snprintf(str, "%u", (int)value);
#endif
		write(str, strlen(str));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(signed int value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%d", value);
#else
		snprintf(str, "%d", value);
#endif
		write(str, strlen(str));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(unsigned int value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%u", (int)value);
#else
		snprintf(str, "%u", value);
#endif
		write(str, strlen(str));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(signed long value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%ld", value);
#else
		snprintf(str, "%ld", value);
#endif
		write(str, strlen(str));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(unsigned long value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%lu", value);
#else
		snprintf(str, "%lu", value);
#endif
		write(str, strlen(str));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(signed long long value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%lld", value);
#else
		snprintf(str, "%lld", value);
#endif
		write(str, strlen(str));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(unsigned long long value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%llu", value);
#else
		snprintf(str, "%llu", value);
#endif
		write(str, strlen(str));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(float value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%f", value);
#else
		snprintf(str, "%f", value);
#endif
		write(str, strlen(str));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(double value)
	{
		char str[128] = { 0 };
#ifdef _MSC_VER
		sprintf_s(str, "%lf", value);
#else
		snprintf(str, "%lf", value);
#endif
		write(str, strlen(str));
		return *this;
	}

	wstring_writer& wstring_writer::operator<<(const char *value)
	{
		write(value, strlen(value));
		return *this;
	}
	wstring_writer& wstring_writer::operator<<(const std::string& value)
	{
		write(value.c_str(), value.size());
		return *this;
	}
}