#include "wmsg.h"
#include "wlog.h"
#include <cstring>

namespace wang
{
	wmsg::wmsg() : m_id(0), m_size(0)
	{

	}

	wmsg::wmsg(uint16 _id, uint32 _size)
		: m_id(_id)
		, m_size(_size)
	{

	}

	void wmsg::fill(const void* p, uint32 len)
	{
		memcpy(m_buf, p, len);
	}

	void wmsg::append_tail(const void* p, uint32 len)
	{
		memcpy(m_buf + m_size, p, len);
		m_size += len;
	}
	void wmsg::remove_tail(void* p, uint32 len)
	{
		if (m_size >= len)
		{
			m_size -= len;
			memcpy(p, m_buf + m_size, len);
		}
		else
		{
			LOG_ERROR << "no enough size";
		}
	}
}
