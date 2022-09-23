#include "wnet_mgr.h"
#include "wmem_pool.h"
#include "wlog.h"
#include "wnet_session.h"
#include "wnet_func.h"
#include "wnet_msg.h"
#include "wnet_msg_reserve.h"
#include "wnet_msg_queue.h"
#include "wnet_msg_statistic.h"
#include <boost/bind.hpp>
#include <sstream>

namespace wang
{
	wnet_mgr::wconnect_para::wconnect_para(wservice& io_service)
		: port(0), timer(io_service)
	{}

	wnet_mgr::wnet_mgr()
		: m_shuting(false)
		, m_max_received_msg_size(1024 * 1024)
		, m_msg_post_placeholder(0)
		, m_send_buf_size(0)
		, m_recv_buf_size(0)
		, m_connect_callback(nullptr)
		, m_disconnect_callback(nullptr)
		, m_msg_callback(nullptr)
		, m_pool_ptr(nullptr)
		, m_work(m_io_service)
		, m_acceptor_ptr(nullptr)
		, m_listening_num(0)
		, m_listening_max(0)
		, m_reconnect_second(0)
		, m_msgs(nullptr)
	{

	}

	wnet_mgr::~wnet_mgr()
	{

	}

	wnet_mgr* wnet_mgr::construct()
	{
		return new wnet_mgr();
	}
	void wnet_mgr::destroy(wnet_mgr* p)
	{
		delete p;
	}

	bool wnet_mgr::init(const std::string& name, uint32 client_session, uint32 server_session
		, uint32 send_buf_size, uint32 recv_buf_size, uint32 pool_size
		, wfailed_cb failed_cb, const std::string& auth_name
		, const std::string& auth_pass, uint32 auth_delay)
	{
		m_name = name;
		m_send_buf_size = send_buf_size;
		m_recv_buf_size = recv_buf_size;

		const uint32 max_session = client_session + server_session;

		if (send_buf_size < MSG_HEADER_SIZE || send_buf_size > 50 * 1024 * 1024)
		{
			LOG_ERROR << "invalid para send_buf_size=" << send_buf_size;
			return false;
		}

		if (recv_buf_size < MSG_HEADER_SIZE || recv_buf_size > 50 * 1024 * 1024)
		{
			LOG_ERROR << "invalid para recv_buf_size=" << recv_buf_size;
			return false;
		}

		m_pool_ptr = new wmem_pool(true);
		if (!m_pool_ptr)
		{
			LOG_ERROR << "memory error";
			return false;
		}

		if (!m_pool_ptr->init(pool_size))
		{
			LOG_ERROR << "pool init error";
			return false;
		}

		m_sessions.reserve(max_session);
		for (uint32 i = 0; i < max_session; ++i)
		{
			m_sessions.push_back(new wnet_session(m_io_service, *this, i));
		}

		if (m_sessions.size() != max_session)
		{
			LOG_ERROR << "new session failed";
			return false;
		}

		for (wnet_session* session : m_sessions)
		{
			if (!session->init(send_buf_size, recv_buf_size))
			{
				return false;
			}
		}

		//服务器初始化
		if (server_session > 0)
		{
			m_available_sessions.reserve(server_session);
			wsessions::reverse_iterator it = m_sessions.rbegin();
			for (uint32 i = 0; i < server_session; ++i)
			{
				m_available_sessions.push_back(*it);
				++it;
			}

			if (m_available_sessions.size() != server_session)
			{
				LOG_ERROR << "m_available_sessions push failed";
				return false;
			}

			m_acceptor_ptr = new wacceptor(m_io_service);
			if (!m_acceptor_ptr)
			{
				LOG_ERROR << "new wacceptor failed";
				return false;
			}
		}

		//客户端初始化
		if (client_session > 0)
		{
			m_paras.reserve(client_session);

			for (uint32 i = 0; i < client_session; ++i)
			{
				m_paras.emplace_back(new wconnect_para(m_io_service));
			}

			if (m_paras.size() != client_session)
			{
				LOG_ERROR << "new socket failed";
				return false;
			}
		}

		m_msgs = new wnet_msg_queue();
		if (!m_msgs)
		{
			LOG_ERROR << "new wnet_msg_queue failed";
			return false;
		}

		return true;
	}

	bool wnet_mgr::startup(uint32 thread_num, const std::string& ip, uint16 port)
	{
		m_listening_max = thread_num;
		if (!m_available_sessions.empty() && !ip.empty() && port)
		{
			//has server
			if (!_start_listen(ip, port, thread_num))
			{
				return false;
			}
		}

		// 创建处理线程
		for (uint32 i = 0; i < thread_num; ++i)
		{
			m_threads.emplace_back(&wnet_mgr::_worker_thread, this);
		}
		return true;
	}

	void wnet_mgr::shut_accept()
	{
		if (!m_shuting)
		{
			m_shuting.store(true);
		}
		if (m_acceptor_ptr && m_acceptor_ptr->is_open())
		{
			werror ec;
			m_acceptor_ptr->cancel(ec);
			m_acceptor_ptr->close(ec);
		}
	}

	void wnet_mgr::shutdown()
	{
		shut_accept();

		if (!m_io_service.stopped())
		{
			// 关闭工作线程
			m_io_service.stop();
		}

		for (std::thread& thread : m_threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

	void wnet_mgr::destroy()
	{
		for (wnet_session* session : m_sessions)
		{
			session->destroy();
			delete session;
		}
		m_sessions.clear();
		m_available_sessions.clear();

		for (wconnect_para* para : m_paras)
		{
			delete para;
		}
		m_paras.clear();

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
			m_pool_ptr = nullptr;
		}

		if (m_acceptor_ptr)
		{
			delete m_acceptor_ptr;
			m_acceptor_ptr = nullptr;
		}

		m_threads.clear();
	}

	bool wnet_mgr::connect_to(uint32 index, const std::string& ip_address, uint16 port)
	{
		if (m_shuting)
		{
			LOG_ERROR << "is shuting " << index;
			return false;
		}

		if (index >= static_cast<uint32>(m_paras.size()))
		{
			LOG_ERROR << "invalid index " << index;
			return false;
		}
		wconnect_para* para_ptr = m_paras[index];

		wnet_session* session_ptr = m_sessions[index];
		if (!session_ptr->is_status_init())
		{
			LOG_ERROR << "session status is not init, index=" << index;
			return false;
		}

		para_ptr->ip.assign(ip_address);
		para_ptr->port = port;

		wendpoint endpoint = wendpoint(boost::asio::ip::address::from_string(ip_address), port);

		session_ptr->set_status_connect();
		session_ptr->get_socket().async_connect(endpoint,
			boost::bind(&wnet_mgr::_handle_connect, this, boost::asio::placeholders::error, index));

		return true;
	}

	bool wnet_mgr::send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size)
	{
		const uint32 index = get_session_index(session_id);
		if (index >= m_sessions.size())
		{
			return false;
		}

		return m_sessions[index]->send_msg(session_id, msg_id, msg_ptr, size);
	}

	bool wnet_mgr::transfer_msg(uint32 session_id, wmsg& msg)
	{
		const uint32 index = get_session_index(session_id);
		if (index >= m_sessions.size())
		{
			return false;
		}

		return m_sessions[index]->send_msg(session_id, wnet_msg::get_net_msg(&msg));
	}

	/**
	* 关闭指定连接
	* @param session_id	连接id
	*/
	void wnet_mgr::close(uint32 session_id)
	{
		const uint32 index = get_session_index(session_id);
		if (index >= m_sessions.size())
		{
			return;
		}

		return m_sessions[index]->close(session_id);
	}

	bool wnet_mgr::is_server_session(uint32 id) const
	{
		return _is_server_index(get_session_index(id));
	}

	void wnet_mgr::process_msg()
	{
		wnet_msg *msg_ptr = m_msgs->dequeue();
		while (msg_ptr)
		{
			wnet_msg* p = msg_ptr;
			msg_ptr = msg_ptr->get_next();
			p->set_next(NULL);
			if (p->get_msg_id() == EMIR_Connect)
			{
				wmsg_connect* q = (wmsg_connect*)p->get_buf();
				m_connect_callback(p->get_session_id(), q->para, q->buf);
				p->free_me();
			}
			else if (p->get_msg_id() == EMIR_Disconnect)
			{
				m_disconnect_callback(p->get_session_id());
				_close_impl(p->get_session_id());
				p->free_me();
			}
			else
			{
				m_msg_callback(p->get_session_id(), p->get_msg());
				p->sub_reference();
			}
		}
	}

	void wnet_mgr::garbage_collect()
	{
		m_pool_ptr->gc();
	}

	std::string wnet_mgr::get_info()
	{
		std::stringstream ss;
		ss << m_name << " info" << '\n';
		ss << "m_listening_num=" << m_listening_num << '\n';
		{
			wlock_guard guard(m_avail_session_mutex);
			ss << "available_sessions=" << m_available_sessions.size() << '\n';
		}
		if (m_pool_ptr)
		{
			ss << m_pool_ptr->get_info();
		}
		return ss.str();
	}
	void wnet_mgr::show_info()
	{
		LOG_INFO << get_info();
	}

	wnet_msg* wnet_mgr::create_msg(uint16 msg_id, uint32 msg_size)
	{
		return wnet_msg::alloc_me(m_pool_ptr, msg_id, msg_size, m_msg_post_placeholder);
	}

	void wnet_mgr::push_msg(wnet_msg* msg_ptr)
	{
		m_msgs->enqueue(msg_ptr);
	}

	void wnet_mgr::static_msg(uint16 msg_id, uint32 msg_size)
	{
		s_net_msg_statistic.count_msg(msg_id, msg_size);
	}
	void wnet_mgr::static_msg(wnet_msg* msg_ptr)
	{
		static_msg(msg_ptr->get_msg_id(), msg_ptr->get_size());
	}

	void wnet_mgr::post_disconnect(wnet_session* session_ptr)
	{
		m_io_service.post(boost::bind(&wnet_mgr::_handle_close, this, session_ptr));
	}

	// 取得一个可用的会话
	wnet_session* wnet_mgr::_get_available_session()
	{
		wnet_session* temp_ptr = NULL;
		{
			wlock_guard guard(m_avail_session_mutex);
			if (m_available_sessions.empty())
			{
				return NULL;
			}
			temp_ptr = m_available_sessions.back();
			m_available_sessions.pop_back();
		}
		return temp_ptr;
	}

	// 归还一个关闭的会话
	void wnet_mgr::_return_session(wnet_session* session_ptr)
	{
		wlock_guard guard(m_avail_session_mutex);
		m_available_sessions.push_back(session_ptr);
	}

	void wnet_mgr::_worker_thread()
	{
		for (;;)
		{
			try
			{
				m_io_service.run();
				LOG_INFO << m_name << " thread exit";
				break;
			}
			catch (boost::system::system_error& e)
			{
				LOG_ERROR << "work thread exception, code = " << e.code().message();
			}
		}
	}

	bool wnet_mgr::_start_listen(const std::string& ip, uint16 port, uint32 thread_num)
	{
		wendpoint endpoint(boost::asio::ip::address::from_string(ip), port);

		werror ec;
		m_acceptor_ptr->open(endpoint.protocol(), ec);

		if (ec)
		{
			LOG_ERROR << "open socket error " << ec.message();
			return false;
		}

		// reuse造成多次绑定成功，可能找不到正确可用的端口
		// 如果两台服务器开在同一台机器会使用同一个端口，造成混乱
		{
			m_acceptor_ptr->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);

			if (ec)
			{
				LOG_ERROR << "set_option reuse_address error, " << ec.message();
				return false;
			}
		}

		m_acceptor_ptr->bind(endpoint, ec);

		if (ec)
		{
			LOG_ERROR << "bind error, " << ec.message();
			return false;
		}

		m_acceptor_ptr->listen(boost::asio::socket_base::max_connections, ec);

		if (ec)
		{
			LOG_ERROR << "listen error, " << ec.message() << ", port = " << port;
			return false;
		}

		for (uint32 i = 0; i < thread_num; ++i)
		{
			_post_accept();
		}

		LOG_INFO << "listen on ip=" << ip << " port=" << port;
		return true;
	}

	void wnet_mgr::_post_accept()
	{
		if (m_listening_num >= m_listening_max || m_shuting)
		{
			return;
		}

		wnet_session* session_ptr = _get_available_session();
		if (!session_ptr)
		{
			return;
		}

		++m_listening_num;

		session_ptr->set_status_accept();

		m_acceptor_ptr->async_accept(session_ptr->get_socket()
			, boost::bind(&wnet_mgr::_handle_accept, this, boost::asio::placeholders::error, session_ptr)
		);
	}

	void wnet_mgr::_handle_accept(const werror& error, wnet_session* session_ptr)
	{
		--m_listening_num;
		wnet_msg* msg_ptr = NULL;
		if (!error)
		{
			msg_ptr = _create_accept_msg(session_ptr->get_socket());
		}

		if (!msg_ptr)
		{
			session_ptr->set_status_init();
			if (session_ptr->get_socket().is_open())
			{
				werror ec;
				session_ptr->get_socket().close(ec);
			}
			_return_session(session_ptr);
			_post_accept();
		}
		else
		{
			_post_accept();
			session_ptr->connected(msg_ptr);
		}
	}

	//连接消息
	wnet_msg* wnet_mgr::_create_accept_msg(const wsocket& socket)
	{
		werror ec;
		wendpoint remote_endpoint = socket.remote_endpoint(ec);

		std::string address = remote_endpoint.address().to_string(ec);
		const uint32 msg_size = static_cast<uint32>(sizeof(wmsg_connect) + address.length());

		//new msg
		wnet_msg* msg_ptr = wnet_msg::alloc_me(m_pool_ptr, EMIR_Connect, msg_size);
		if (!msg_ptr)
		{
			LOG_ERROR << "alloc msg EMIR_ListenConnect failed";
			return NULL;
		}

		wmsg_connect* connect_msg_ptr = (wmsg_connect*)(msg_ptr->get_buf());
		//connect_msg_ptr->port = remote_endpoint.port();
		connect_msg_ptr->para = static_cast<uint32>(address.length());
		memcpy(connect_msg_ptr->buf, address.data(), address.length());
		connect_msg_ptr->buf[address.length()] = 0;
		return msg_ptr;
	}

	void wnet_mgr::_handle_connect(const werror& error, uint32 index)
	{
		wconnect_para* para_ptr = m_paras[index];
		wnet_session* session_ptr = m_sessions[index];

		if (error)
		{
			if (session_ptr->get_socket().is_open())
			{
				werror ec;
				session_ptr->get_socket().close(ec);
			}

			if (m_reconnect_second != 0)
			{
				//连接失败 重连
				LOG_WARN << "connect to " << para_ptr->ip << ":" << para_ptr->port
					<< " failed, code=" << error.message() << ", reconnect " << m_reconnect_second << " second later";

				_post_reconnect(para_ptr, index, m_reconnect_second);
				return;
			}

			//不需要重连, 回调连接失败
			wnet_msg* msg_ptr = _create_connect_msg(error.value());
			if (!msg_ptr)
			{
				//回调连接失败又失败了, 内存不足, 5秒后重连
				LOG_WARN << "connect to " << para_ptr->ip << ":" << para_ptr->port
					<< " failed, memory not enough, retry 5 second later";
				_post_reconnect(para_ptr, index, 5);
				return;
			}

			//不需要重连, 回调连接失败
			session_ptr->set_status_init();
			push_msg(msg_ptr);
		}
		else
		{
			//连接成功
			wnet_msg* msg_ptr = _create_connect_msg(0);
			if (!msg_ptr)
			{
				// 回调连接成功失败
				if (session_ptr->get_socket().is_open())
				{
					werror ec;
					session_ptr->get_socket().close(ec);
				}

				//内存不足, 5秒后重连
				LOG_WARN << "connect to " << para_ptr->ip << ":" << para_ptr->port
					<< " success, but memory not enough, retry 5 second later";
				_post_reconnect(para_ptr, index, 5);
				return;
			}

			//成功, 返回
			session_ptr->connected(msg_ptr);
		}
	}
	void wnet_mgr::_post_reconnect(wconnect_para* para_ptr, uint32 index, uint32 seconds)
	{
		if (m_shuting)
		{
			m_sessions[index]->set_status_init();
			return;
		}

		para_ptr->timer.expires_from_now(boost::posix_time::seconds(seconds));
		para_ptr->timer.async_wait(boost::bind(&wnet_mgr::_handle_reconnect, this, index));
	}
	void wnet_mgr::_handle_reconnect(uint32 index)
	{
		wconnect_para* para_ptr = m_paras[index];
		wnet_session* session_ptr = m_sessions[index];

		if (m_shuting)
		{
			session_ptr->set_status_init();
			return;
		}
		wendpoint endpoint = wendpoint(boost::asio::ip::address::from_string(para_ptr->ip), para_ptr->port);

		session_ptr->get_socket().async_connect(endpoint,
			boost::bind(&wnet_mgr::_handle_connect, this, boost::asio::placeholders::error, index));
	}

	//连接消息
	wnet_msg* wnet_mgr::_create_connect_msg(uint32 error_code)
	{
		const uint32 msg_size = sizeof(wmsg_connect);

		wnet_msg* msg_ptr = wnet_msg::alloc_me(m_pool_ptr, EMIR_Connect, msg_size);
		if (!msg_ptr)
		{
			LOG_ERROR << "alloc msg EMIR_ConnectTo failed";
			return NULL;
		}

		wmsg_connect* connect_msg_ptr = (wmsg_connect*)(msg_ptr->get_buf());
		connect_msg_ptr->para = error_code;
		connect_msg_ptr->buf[0] = 0;
		return msg_ptr;
	}

	void wnet_mgr::_handle_close(wnet_session* session_ptr)
	{
		wnet_msg* msg_ptr = wnet_msg::alloc_me(m_pool_ptr, EMIR_Disconnect, 0);
		if (!msg_ptr)
		{
			LOG_ERROR << "alloc msg EMIR_Disconnect failed, name=" << get_name();
			post_disconnect(session_ptr);
			return;
		}
		const uint32 session_id = session_ptr->get_id();
		msg_ptr->set_session_id(session_id);
		session_ptr->reset();
		push_msg(msg_ptr);
	}

	void wnet_mgr::_close_impl(uint32 session_id)
	{
		const uint32 index = get_session_index(session_id);
		if (_is_server_index(index))
		{
			_return_session(m_sessions[index]);
			_post_accept();
		}
		else if (m_reconnect_second > 0)
		{
			//重连
			wconnect_para* para_ptr = m_paras[index];
			m_sessions[index]->set_status_connect();
			_post_reconnect(para_ptr, index, m_reconnect_second);
		}
	}

	bool wnet_mgr::_is_server_index(uint32 index) const
	{
		return (index >= static_cast<uint32>(m_paras.size()));
	}
}