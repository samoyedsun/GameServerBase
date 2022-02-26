#ifndef W_NET_MSG_QUEUE_H
#define W_NET_MSG_QUEUE_H

#include <mutex>

namespace wang {

	class wnet_msg;

	class wnet_msg_queue
	{
	public:
		wnet_msg_queue();
		~wnet_msg_queue();

		void enqueue(wnet_msg* msg_ptr);
		wnet_msg* dequeue();
		void clear();

	private:
		std::mutex	m_mutex;
		wnet_msg*	m_head_ptr;
		wnet_msg*	m_tail_ptr;
	};

} // namespace wang

#endif // W_NET_MSG_QUEUE_H
