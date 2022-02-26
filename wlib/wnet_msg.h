#ifndef W_NET_MSG_H
#define W_NET_MSG_H

#include "wmsg.h"
#include <atomic>

namespace wang
{
	class wmem_pool;

#pragma pack(push,1)

	class wnet_msg
	{
	private:
		explicit wnet_msg(wmem_pool* pool_ptr, uint16 msg_id, uint32 msg_size);
		~wnet_msg();

	public:
		static wnet_msg* alloc_me(wmem_pool* pool_ptr, uint16 msg_id, uint32 msg_size
			, uint32 placeholder = 0);
		void free_me();

	public:
		wnet_msg* get_next() { return m_next_ptr; }
		void set_next(wnet_msg* value) { m_next_ptr = value; }

		uint32 get_session_id() const { return m_session_id; }
		void set_session_id(uint32 value) { m_session_id = value; }

		void set_reference(uint16 value) { m_reference.store(value); }
		void sub_reference();
		void add_reference() { m_reference.fetch_add(1); }

		const wmsg& get_msg() const { return m_msg; }
		wmsg& get_msg() { return m_msg; }

		const void* get_msg_buf() const { return &m_msg; }
		uint32 get_msg_size() const { return MSG_HEADER_SIZE + m_msg.get_size(); }

		uint16 get_msg_id() const { return m_msg.get_id(); }
		void set_msg_id(uint16 value) { m_msg.set_id(value); }

		uint32 get_size() const { return m_msg.get_size(); }
		void set_size(uint32 value) { m_msg.set_size(value); }

		const char* get_buf() const { return m_msg.get_buf(); }
		char* get_buf() { return m_msg.get_buf(); }

		void fill(const void* p, uint32 len) { m_msg.fill(p, len); }

		//p÷∏œÚm_buffer
		static wnet_msg* get_net_msg(wmsg* p);

	private:
		wnet_msg(const wnet_msg&);
		wnet_msg& operator=(const wnet_msg&);

	private:
		wmem_pool*			m_pool_ptr;
		wnet_msg*			m_next_ptr;
		uint32				m_session_id;
		std::atomic_ushort	m_reference;
		wmsg				m_msg;
	};

#pragma pack(pop)
}

#endif //W_NET_MSG_H
