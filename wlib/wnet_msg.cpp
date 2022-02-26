#include "wnet_msg.h"
#include "wmem_pool.h"

namespace wang
{
	//static void check_net_msg_size()
	//{
	static_assert(sizeof(wnet_msg) == 29 || sizeof(wnet_msg) == 21
		, "sizeof(wnet_msg) != 29 && sizeof(wnet_msg) != 21");
	//}

	wnet_msg::wnet_msg(wmem_pool* pool_ptr, uint16 msg_id, uint32 msg_size)
		: m_pool_ptr(pool_ptr)
		, m_next_ptr(nullptr)
		, m_session_id(0)
		, m_reference(1)
		, m_msg(msg_id, msg_size)
	{

	}

	wnet_msg::~wnet_msg()
	{
	}

	wnet_msg* wnet_msg::alloc_me(wmem_pool* pool_ptr, uint16 msg_id, uint32 msg_size, uint32 placeholder)
	{
		void* p = pool_ptr->alloc(sizeof(wnet_msg) - 1 + msg_size + placeholder);
		if (!p)
		{
			return nullptr;
		}
		wnet_msg* q = new (p) wnet_msg(pool_ptr, msg_id, msg_size);
		return q;
	}

	void wnet_msg::free_me()
	{
		this->~wnet_msg();
		m_pool_ptr->free(this);
	}

	void wnet_msg::sub_reference()
	{
		if (m_reference.fetch_sub(1) == 1)
		{
			free_me();
		}
	}

	wnet_msg* wnet_msg::get_net_msg(wmsg* p)
	{
		return reinterpret_cast<wnet_msg*>(
			reinterpret_cast<char*>(p) - (sizeof(wnet_msg) - sizeof(wmsg))
			);
	}
}
