#ifndef W_NET_FUNC_H
#define W_NET_FUNC_H

#include "wnet_type.h"

namespace wang
{
	inline uint32 get_session_index(uint32 id) { return id & 0x0000FFFF; }

	inline uint32 get_session_serial(uint32 id) { return (id & 0xFFFF0000) >> 16; }
}

#endif //W_NET_FUNC_H
