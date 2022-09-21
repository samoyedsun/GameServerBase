#ifndef W_STRING_WRITER_H
#define W_STRING_WRITER_H

#include <string>
#include "wbuffer.h"

namespace wang {

	class wstring_writer : public wbuffer
	{
	public:
		wstring_writer(char * buf, unsigned int size);

	public:
		wstring_writer& operator<<(bool);
		wstring_writer& operator<<(char);

		wstring_writer& operator<<(signed char);
		wstring_writer& operator<<(unsigned char);

		wstring_writer& operator<<(signed short);
		wstring_writer& operator<<(unsigned short);

		wstring_writer& operator<<(signed int);
		wstring_writer& operator<<(unsigned int);

		wstring_writer& operator<<(signed long);
		wstring_writer& operator<<(unsigned long);

		wstring_writer& operator<<(signed long long);
		wstring_writer& operator<<(unsigned long long);

		wstring_writer& operator<<(float);
		wstring_writer& operator<<(double);

		wstring_writer& operator<<(const char *);
		wstring_writer& operator<<(const std::string&);
	};

}
#endif // W_STRING_WRITER_H
