#include "wbuffer.h"
#include <iostream>

namespace wang {

	wbuffer::wbuffer(char * buf, unsigned int size) :
		m_buf(buf),
		m_buf_size(size),
		m_pos(0),
		m_error(false)
	{
	}

	void wbuffer::set_buf(char * buf, unsigned int size)
	{
		m_buf = buf;
		m_buf_size = size;
	}

	void wbuffer::reset()
	{
		m_buf = NULL;
		m_buf_size = 0;
		m_pos = 0;
		m_error = false;
	}

	void wbuffer::write(const void* buf, unsigned int len)
	{
		if (!m_error && m_pos + len <= m_buf_size)
		{
			memcpy(m_buf + m_pos, buf, len);
			m_pos += len;
			return;
		}
		set_error();
	}

	void wbuffer::write(unsigned int pos, const void* buf, unsigned  int len)
	{
		if (!m_error && pos + len <= m_buf_size)
		{
			memcpy(m_buf + pos, buf, len);
			m_pos += len;
			return;
		}
		set_error();
	}
	
	void wbuffer::skip(unsigned int len)
	{
		m_pos += len;
	}

}
