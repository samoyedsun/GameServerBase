/********************************************************************
created:	2016/08/13

author:		wanghaibo

purpose:	整数转字符串函数
*********************************************************************/

#ifndef W_INT2STR_H_ 
#define W_INT2STR_H_

namespace wang
{
	//convert int to decimal string, add '\0' at end
	template<typename T>
	int int2str_dec(char* buf, T value);

	//convert int to hexadecimal string, add '\0' at end
	template<typename T>
	int int2str_hex(char* buf, T value);

	//src buf to dst_buf, length of dst buf must > 2 * buf_len, will add '\0' at end
	void byte2hex(char* dst_buf, const unsigned char *src_buf, int buf_len);
}

#endif  // W_INT2STR_H_

