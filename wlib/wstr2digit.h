#ifndef W_STR2DIGIT_H
#define W_STR2DIGIT_H

namespace wang
{
	//buf must be zero end
	template<typename T>
	void str2digit_dec(const char* buf, T& value);
}

#endif  // W_STR2DIGIT_H

