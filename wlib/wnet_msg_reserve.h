#ifndef W_NET_MSG_RESERVE_H
#define W_NET_MSG_RESERVE_H

#include "wnet_type.h"

namespace wang {

	enum EMsgIDReserve
	{
		EMIR_Connect = 1, //��������
		EMIR_Disconnect,

		EMIR_Max,
	};

	/**
	* ������������ �� ������������
	* buf[0] == 0 ��ʾ������������, �����ʾ������������
	* ������������ para��ʾbuf����, buf��ʾip��ַ
	* ������������ para��ʾ������,  buf[0] == 0
	*/
	struct wmsg_connect
	{
		uint32 para;
		char buf[1];
	};

} // namespace wang

#endif // W_MSG_RESERVE_H
