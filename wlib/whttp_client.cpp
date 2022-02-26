#include "whttp_client.h"
#include "whttp_session.h"
#include "wlog.h"
#include "wmem_pool.h"
#include "wnet_msg_queue.h"
#include "wnet_msg.h"
#include <functional>

namespace wang {

	whttp_client::whttp_client() : m_work(m_io_service)
		, m_is_working(false)
		, m_stop(true)
		, m_max_session(0)
		, m_msg_handler(nullptr)
		, m_pool_ptr(nullptr)
		, m_msgs(nullptr)
	{

	}

	whttp_client::~whttp_client()
	{
		//destroy();
	}

	bool whttp_client::init(wmsg_handler msg_handler, uint32 max_session, uint32 send_buf_size, uint32 recv_buf_size)
	{
		m_msg_handler = msg_handler;

		m_max_session = max_session;

		m_sessions.reserve(max_session);

		for (uint32 i = 0; i < m_max_session; ++i)
		{
			whttp_session* p = new whttp_session(m_io_service);

			if (!p)
			{
				ERROR_EX_LOG("alloc whttp_session memory error!");
				return false;
			}

			if (!p->init(send_buf_size, recv_buf_size))
			{
				return false;
			}

			m_sessions.push_back(p);

			_return_session(p);
		}

		if (m_sessions.size() != m_max_session)
		{
			ERROR_EX_LOG("m_sessions.size() != m_max_session!");
			return false;
		}

		m_pool_ptr = new wmem_pool(true);
		if (!m_pool_ptr)
		{
			ERROR_EX_LOG("new wmem_pool failed");
			return false;
		}
		m_pool_ptr->init();

		m_msgs = new wnet_msg_queue();
		if (!m_msgs)
		{
			ERROR_EX_LOG("new wnet_msg_queue failed");
			return false;
		}

		return true;
	}

	void whttp_client::destroy()
	{
		for (whttp_session* session : m_sessions)
		{
			session->destroy();
			delete session;
		}
		m_sessions.clear();
		m_available_sessions.clear();

		if (m_msgs)
		{
			wnet_msg *p = m_msgs->dequeue();
			wnet_msg* tmp = nullptr;
			while (p)
			{
				tmp = p;
				p = p->get_next();
				m_pool_ptr->free(tmp);
			}

			delete m_msgs;
			m_msgs = nullptr;
		}

		if (m_pool_ptr)
		{
			m_pool_ptr->destroy();
			delete m_pool_ptr;
			m_pool_ptr = NULL;
		}
	}

	bool whttp_client::startup(const char* name_ptr)
	{
		// 开启状态
		m_is_working = true;

		// 创建完成包处理线程
		m_stop = false;

		std::thread t(&whttp_client::_work, this);

		m_thread.swap(t);

		return true;
	}

	void whttp_client::shutdown()
	{
		// 设置关闭状态
		m_is_working = false;

		m_stop = true;

		// 关闭工作线程
		m_io_service.stop();

		if (m_thread.joinable())
		{
			m_thread.join();
		}

		for (whttp_session* session : m_sessions)
		{
			session->close();
		}
	}

	bool whttp_client::http_request(const std::string& host, const char* request_ptr, uint32 request_size, uint32 client_id, uint16 msg_id, uint32 time_out_sec)
	{
		if (!m_is_working)
		{
			return false;
		}

		whttp_session* session_ptr = _get_available_session();

		if (!session_ptr)
		{
			return false;
		}

		bool ret = session_ptr->http_request(host, request_ptr, request_size, time_out_sec
			, std::bind(&whttp_client::on_response, this, client_id, msg_id, std::placeholders::_1)
		);

		if (!ret)
		{
			if (session_ptr->get_socket().is_open())
			{
				boost::system::error_code ec;
				session_ptr->get_socket().close(ec);
			}

			_return_session(session_ptr);
			return false;
		}

		return true;
	}

	void whttp_client::on_response(uint32 client_id, uint16 msg_id, whttp_session* session_ptr)
	{
		wnet_msg* p = wnet_msg::alloc_me(m_pool_ptr, msg_id, session_ptr->get_recv_size());
		if (!p)
		{
			ERROR_EX_LOG("memory alloc failed");
			return;
		}

		p->set_session_id(client_id);
		p->fill(session_ptr->get_recv_buf(), session_ptr->get_recv_size());
		m_msgs->enqueue(p);

		_return_session(session_ptr);
	}

	uint32 whttp_client::get_avaliable()
	{
		wlock_guard guard(m_mutex);
		return static_cast<uint32>(m_available_sessions.size());
	}

	void whttp_client::process_msg()
	{
		wnet_msg *p = m_msgs->dequeue();

		while (p)
		{
			m_msg_handler(p->get_session_id(), p->get_msg_id()
				, p->get_buf(), p->get_size());
			wnet_msg *tmp_msg = p;
			p = p->get_next();
			tmp_msg->free_me();
		}
	}

	void whttp_client::garbage_collect()
	{
		if (m_pool_ptr)
		{
			m_pool_ptr->gc();
		}
	}

	void whttp_client::_work()
	{
		while (!m_stop)
		{
			try
			{
				m_io_service.run();
				SYSTEM_LOG("nc_client_asio work thread exit");
				break;
			}
			catch (boost::system::system_error& e)
			{
				ERROR_EX_LOG("work thread exception, code = %d", e.code().value());
			}
		}
	}

	// 取得一个可用的会话
	whttp_session* whttp_client::_get_available_session()
	{
		wlock_guard guard(m_mutex);

		if (!m_is_working || m_available_sessions.empty())
		{
			return NULL;
		}

		whttp_session* temp_ptr = m_available_sessions.front();
		m_available_sessions.pop_front();
		return temp_ptr;
	}

	// 归还一个关闭的会话
	void whttp_client::_return_session(whttp_session* session_ptr)
	{
		wlock_guard guard(m_mutex);

		if (!session_ptr)
		{
			return;
		}

		m_available_sessions.push_back(session_ptr);
	}

} // namespace wang
