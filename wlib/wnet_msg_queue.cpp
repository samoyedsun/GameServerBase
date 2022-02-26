#include "wnet_msg_queue.h"
#include "wmem_pool.h"
#include "wnet_msg.h"

namespace wang {

	wnet_msg_queue::wnet_msg_queue()
		: m_head_ptr(NULL)
		, m_tail_ptr(NULL)
	{}

	wnet_msg_queue::~wnet_msg_queue()
	{
	}

	void wnet_msg_queue::enqueue(wnet_msg* msg_ptr)
	{
		std::lock_guard<std::mutex> guard(m_mutex);

		// 加入队列
		if (m_tail_ptr)
		{
			m_tail_ptr->set_next(msg_ptr);
			m_tail_ptr = msg_ptr;
		}
		else
		{
			m_head_ptr = m_tail_ptr = msg_ptr;
		}
	}

	wnet_msg* wnet_msg_queue::dequeue()
	{
		if (!m_head_ptr)
		{
			return NULL;
		}
		else
		{
			std::lock_guard<std::mutex> guard(m_mutex);
			wnet_msg* msg_ptr = m_head_ptr;
			m_head_ptr = NULL;
			m_tail_ptr = NULL;
			return msg_ptr;
		}
	}

	void wnet_msg_queue::clear()
	{
		wnet_msg* msg_ptr = NULL;
		std::lock_guard<std::mutex> guard(m_mutex);

		while (m_head_ptr)
		{
			msg_ptr = m_head_ptr;
			m_head_ptr = m_head_ptr->get_next();
			msg_ptr->free_me();
		}

		m_head_ptr = NULL;
		m_tail_ptr = NULL;
	}

} // namespace wang
