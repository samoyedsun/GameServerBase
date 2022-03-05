#ifndef W_DIGIT2STR_H
#define W_DIGIT2STR_H

namespace wang
{
	//convert int to decimal string, add '\0' at end
	template<typename T>
	int digit2str_dec(char* buf, int buf_size, T value);

	//convert int to hexadecimal string, add '\0' at end
	template<typename T>
	int int2str_hex(char* buf, T value);

	//src buf to dst_buf, length of dst buf must > 2 * buf_len, will add '\0' at end
	void byte2hex(char* dst_buf, const unsigned char *src_buf, int buf_len);
}

#endif  // W_DIGIT2STR_H

