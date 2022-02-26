#ifndef W_MSG_H
#define W_MSG_H

#include "wnet_type.h"

namespace wang
{
#pragma pack(push,1)
	class wmsg
	{
	public:
		wmsg();
		explicit wmsg(uint16 _id, uint32 _size);

	public:
		uint16 get_id() const { return m_id; }
		void set_id(uint16 value) { m_id = value; }

		uint32 get_size() const { return m_size; }
		void set_size(uint32 value) { m_size = value; }

		const char* get_buf() const { return m_buf; }
		char* get_buf() { return m_buf; }

		void fill(const void* p, uint32 len);

		void append_tail(const void* p, uint32 len);
		void remove_tail(void* p, uint32 len);

	private:
		uint16 m_id;
		uint32 m_size;
		char   m_buf[1];
	};
#pragma pack(pop)

	static const uint32 MSG_ID_SIZE = sizeof(uint16);
	static const uint32 MSG_HEADER_SIZE = MSG_ID_SIZE + sizeof(uint32);
}

#endif //W_MSG_H
