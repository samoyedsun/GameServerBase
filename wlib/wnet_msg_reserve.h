#ifndef W_NET_MSG_RESERVE_H
#define W_NET_MSG_RESERVE_H

#include "wnet_type.h"

namespace wang {

	enum EMsgIDReserve
	{
		EMIR_Connect = 1, //建立连接
		EMIR_Disconnect,

		EMIR_Max,
	};

	/**
	* 监听建立连接 或 主动建立连接
	* buf[0] == 0 表示主动建立连接, 否则表示监听建立连接
	* 监听建立连接 para表示buf长度, buf表示ip地址
	* 主动建立连接 para表示错误码,  buf[0] == 0
	*/
	struct wmsg_connect
	{
		uint32 para;
		char buf[1];
	};

} // namespace wang

#endif // W_MSG_RESERVE_H
