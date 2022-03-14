#include "whttp_client.h"
#include "whttp_error_code.h"
#include "wlog.h"
#include "wmem_pool.h"
#include <boost/bind.hpp>

namespace wang {


	/**
	*	whttp_session 是http网络传输会话(基于boost.asio)
	*/
	class whttp_session
	{
	public:
		typedef std::function<void(int, const void*, uint32)> wresponse;

	public:
		whttp_session(boost::asio::io_service& io_service);
		~whttp_session();

		// 回话初始化
		bool init(uint32 send_buf_size, uint32 recv_buf_size);

		void destroy();

		bool is_closeing() const { return m_is_closeing; }

		/**
		* 异步发送http请求
		* @param host			host地址
		* @param method		方法
		* @param param			参数
		* @param client_id		请求标识id
		* @param msg_id		返回消息id
		* @param time_out_sec	超时时间，单位s
		* @return				true请求成功, false表示请求投递失败
		*/
		bool http_request(const std::string& host, const char* request_ptr, uint32 request_size
			, uint32 time_out_sec, wresponse func);

		void close();

		const char* get_recv_buf() const { return m_recv_buf; }
		uint32 get_recv_size() const { return m_recv_size; }

	public:
		boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

		// 地址解析回调
		void handle_resolve(const boost::system::error_code& err
			, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

		// 连接回调
		void handle_connect(const boost::system::error_code& err);

		// 请求发送完回调
		void handle_write_request(const boost::system::error_code& err);

		// 内容读取回调
		void handle_read_content(const boost::system::error_code& err, std::size_t bytes_transferred);

		// 时钟回调
		void check_deadline();

	private:

		// 关闭指定连接
		void _close();

		// 尝试释放资源
		void _release();

		// 处理结果
		void _handle_result(uint32 error_code, std::string msg);

	private:
		whttp_session(const whttp_session& rht);
		whttp_session& operator=(const whttp_session& rht);

	private:
		bool							m_is_requesting;	// 是否还有异步请求
		bool							m_is_timing;		// 是否正在倒计时
		bool							m_is_closeing;		// 是否已经调用过socket close
		boost::asio::ip::tcp::resolver	m_resolver;			// 地址解析
		boost::asio::deadline_timer		m_timer;			// 时钟
		boost::asio::ip::tcp::socket	m_socket;			// session的socket资源

															//发送缓冲
		char*							m_send_buf;
		uint32							m_send_buf_size;
		uint32							m_send_size;

		//接收缓冲
		char*							m_recv_buf;
		uint32							m_recv_buf_size;
		uint32							m_recv_size;

		wresponse						m_response;			// 回调
	};

	whttp_session::whttp_session(boost::asio::io_service& io_service)
		: m_is_requesting(false)
		, m_is_timing(false)
		, m_is_closeing(false)
		, m_resolver(io_service)
		, m_timer(io_service)
		, m_socket(io_service)
		, m_send_buf(nullptr)
		, m_send_buf_size(0)
		, m_send_size(0)
		, m_recv_buf(nullptr)
		, m_recv_buf_size(0)
		, m_recv_size(0)
		, m_response(nullptr)
	{
	}

	whttp_session::~whttp_session()
	{

	}

	void whttp_session::destroy()
	{
		m_is_requesting = false;
		m_is_timing = false;
		m_is_closeing = false;

		if (m_send_buf)
		{
			delete[] m_send_buf;
			m_send_buf = nullptr;
		}
		m_send_size = 0;

		if (m_recv_buf)
		{
			delete[] m_recv_buf;
			m_recv_buf = nullptr;
		}
		m_recv_size = 0;

		m_response = nullptr;
	}

	bool whttp_session::init(uint32 send_buf_size, uint32 recv_buf_size)
	{
		if (recv_buf_size < 4)
		{
			LOG_ERROR << "m_recv_buf_size < 4";
			return false;
		}

		m_send_buf = new char[send_buf_size];
		if (!m_send_buf)
		{
			LOG_ERROR << "new m_send_buf failed";
			return false;
		}

		m_recv_buf = new char[recv_buf_size];
		if (!m_send_buf)
		{
			LOG_ERROR << "new m_receive_buf failed";
			return false;
		}

		m_send_buf_size = send_buf_size;
		m_recv_buf_size = recv_buf_size;

		return true;
	}

	bool whttp_session::http_request(const std::string& host, const char* request_ptr
		, uint32 request_size, uint32 time_out_sec, wresponse func)
	{
		// 检查状态
		if (m_is_requesting || m_is_closeing)
		{
			LOG_ERROR << "m_is_requesting";
			return false;
		}

		if (request_size > m_send_buf_size)
		{
			LOG_ERROR << "request_size too big, " << request_size << " > " << m_send_buf_size;
			return false;
		}

		m_is_requesting = true;
		m_is_timing = true;

		m_response = func;
		m_recv_size = 0;

		// 设置超时
		m_timer.expires_from_now(boost::posix_time::seconds(time_out_sec));
		m_timer.async_wait(boost::bind(&whttp_session::check_deadline, this));

		memcpy(m_send_buf, request_ptr, request_size);
		m_send_size = request_size;

		std::string _host(host);
		std::string _service("http");
		std::string::size_type n = _host.find_first_of(':');
		if (n != std::string::npos)
		{
			_service = _host.substr(n + 1);
			_host = _host.substr(0, n);
		}

		boost::asio::ip::tcp::resolver::query query(_host, _service);
		m_resolver.async_resolve(query, boost::bind(&whttp_session::handle_resolve, this
			, boost::asio::placeholders::error, boost::asio::placeholders::iterator));
		return true;
	}

	void whttp_session::close()
	{
		if (m_is_timing)
		{
			boost::system::error_code ec;
			m_timer.cancel(ec);
		}
		if (!m_is_closeing)
		{
			m_is_closeing = true;
			if (m_socket.is_open())
			{
				boost::system::error_code ec;
				m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				m_socket.close(ec);
			}
		}
	}

	void whttp_session::handle_resolve(const boost::system::error_code& err
		, boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
	{
		if (err)
		{
			_handle_result(EHRCode_Resolve, err.message());
			return;
		}

		if (!m_is_timing)
		{
			_handle_result(EHRCode_Timeout, "time out");
			return;
		}

		boost::asio::async_connect(m_socket
			, endpoint_iterator
			, boost::bind(&whttp_session::handle_connect, this, boost::asio::placeholders::error));
	}

	void whttp_session::handle_connect(const boost::system::error_code& err)
	{
		if (err)
		{
			_handle_result(EHRCode_Connect, err.message());
			return;
		}

		if (!m_is_timing)
		{
			_handle_result(EHRCode_Timeout, "time out");
			return;
		}

		boost::asio::async_write(m_socket
			, boost::asio::buffer(m_send_buf, m_send_size)
			, boost::bind(&whttp_session::handle_write_request, this, boost::asio::placeholders::error));
	}

	void whttp_session::handle_write_request(const boost::system::error_code& err)
	{
		if (err)
		{
			_handle_result(EHRCode_Write, err.message());
			return;
		}

		if (!m_is_timing)
		{
			_handle_result(EHRCode_Timeout, "time out");
			return;
		}

		*reinterpret_cast<int32*>(m_recv_buf) = EHRCode_Succ;
		m_recv_size = 4;
		boost::asio::async_read(m_socket
			, boost::asio::buffer(m_recv_buf + m_recv_size, m_recv_buf_size - m_recv_size)
			, boost::asio::transfer_all()
			, boost::bind(&whttp_session::handle_read_content, this, boost::asio::placeholders::error
				, boost::asio::placeholders::bytes_transferred));
	}

	void whttp_session::handle_read_content(const boost::system::error_code& err, std::size_t bytes_transferred)
	{
		if (err == boost::asio::error::eof)
		{
			m_recv_size += static_cast<uint32>(bytes_transferred);
			_handle_result(EHRCode_Succ, "");
			return;
		}

		if (err)
		{
			_handle_result(EHRCode_Err, err.message());
			return;
		}

		if (!m_is_timing)
		{
			_handle_result(EHRCode_Timeout, "time out");
			return;
		}

		m_recv_size += static_cast<uint32>(bytes_transferred);
		_handle_result(EHRCode_Succ, "");
	}

	void whttp_session::check_deadline()
	{
		m_is_timing = false;

		if (!m_is_closeing)
		{
			m_is_closeing = true;
			boost::system::error_code ec;
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			m_socket.close(ec);
		}

		_release();
	}

	void whttp_session::_release()
	{
		// 尝试判断
		if (!m_is_closeing || m_is_requesting || m_is_timing)
		{
			return;
		}

		// 设置session状态
		m_is_closeing = false;

		// 归还session
		if (m_response)
		{
			int32 code = *(int32 *)m_recv_buf;
			m_response(code, m_recv_buf + 4, m_recv_size);
		}
	}

	void whttp_session::_close()
	{
		if (!m_is_closeing)
		{
			m_is_closeing = true;

			boost::system::error_code ec;
			m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			m_socket.close(ec);
			m_timer.cancel();
		}

		// 尝试释放资源
		_release();
	}

	// 处理结果
	void whttp_session::_handle_result(uint32 status_code, const std::string error_msg)
	{
		if (status_code != EHRCode_Succ)
		{
			*reinterpret_cast<uint32*>(m_recv_buf) = status_code;
			m_recv_size = 4;
			if (error_msg.length() < m_recv_buf_size - 4)
			{
				memcpy(m_recv_buf + 4, error_msg.c_str(), error_msg.length());
				m_recv_size += static_cast<uint32>(error_msg.length());

				memset(m_recv_buf + 4 + error_msg.length(), 0, 1);
				m_recv_size += static_cast<uint32>(1);
			}
		}

		// 修改请求计数
		m_is_requesting = false;

		// 关闭连接
		_close();
	}

	class whttp_msg
	{
	private:
		explicit whttp_msg(wmem_pool* pool_ptr, uint32 size)
			: m_size(size)
			, m_code(0)
			, m_pool_ptr(pool_ptr)
			, m_next_ptr(NULL)
		{
		}

		whttp_msg::~whttp_msg()
		{
		}

	public:
		static whttp_msg* whttp_msg::alloc_me(wmem_pool* pool_ptr, uint32 size, uint32 placeholder = 0)
		{
			void* p = pool_ptr->alloc(sizeof(whttp_msg) - 1 + size + placeholder);
			if (!p)
			{
				return nullptr;
			}
			return new (p) whttp_msg(pool_ptr, size);
		}

		void free_me()
		{
			this->~whttp_msg();
			m_pool_ptr->free(this);
		}

	public:
		whttp_msg* get_next() { return m_next_ptr; }
		void set_next(whttp_msg* value) { m_next_ptr = value; }

		whttp_client::wmsg_handler get_handler() { return m_handler; }
		void set_handler(whttp_client::wmsg_handler handler) { m_handler = handler; }

		int get_code() { return m_code; }
		void set_code(int code) { m_code = code; }

		uint32 get_size() const { return m_size; }
		const char* get_buf() const { return m_buf; }

		void fill(const void* p, uint32 len)
		{
			memcpy(m_buf, p, len);
		}

	private:
		wmem_pool*						m_pool_ptr;
		whttp_msg*						m_next_ptr;
		whttp_client::wmsg_handler		m_handler;	// 回调函数
		int								m_code;		// 状态码
		uint32							m_size;		// 数据大小
		char							m_buf[1];	// 数据内容
	};

	class whttp_msg_queue
	{
	public:
		whttp_msg_queue()
			: m_head_ptr(NULL)
			, m_tail_ptr(NULL)
		{}
		~whttp_msg_queue()
		{
		}

		void enqueue(whttp_msg* msg_ptr)
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
		whttp_msg* dequeue()
		{
			if (!m_head_ptr)
			{
				return NULL;
			}
			else
			{
				std::lock_guard<std::mutex> guard(m_mutex);
				whttp_msg* msg_ptr = m_head_ptr;
				m_head_ptr = NULL;
				m_tail_ptr = NULL;
				return msg_ptr;
			}
		}

		void clear()
		{
			whttp_msg* msg_ptr = NULL;
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

	private:
		std::mutex	m_mutex;
		whttp_msg*	m_head_ptr;
		whttp_msg*	m_tail_ptr;
	};

	whttp_client::whttp_client() : m_work(m_io_service)
		, m_is_working(false)
		, m_stop(true)
		, m_max_session(0)
		, m_pool_ptr(nullptr)
		, m_msgs(nullptr)
	{

	}

	whttp_client::~whttp_client()
	{
		//destroy();
	}

	bool whttp_client::init(uint32 max_session, uint32 send_buf_size, uint32 recv_buf_size)
	{
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

		m_msgs = new whttp_msg_queue();
		if (!m_msgs)
		{
			ERROR_EX_LOG("new whttp_msg_queue failed");
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
			whttp_msg *p = m_msgs->dequeue();
			whttp_msg* tmp = nullptr;
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

	bool whttp_client::http_request(const std::string& ip, const std::string port
		, const char* request_ptr, uint32 request_size, wmsg_handler func, uint32 time_out_sec)
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

		bool ret = session_ptr->http_request(ip + ":" + port, request_ptr, request_size, time_out_sec,
			std::bind(&whttp_client::on_response, this, func, session_ptr,
				std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
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

	void whttp_client::on_response(wmsg_handler func, whttp_session* session_ptr
		, int code, const void* buf, uint32 size)
	{
		whttp_msg* p = whttp_msg::alloc_me(m_pool_ptr, size);
		if (!p)
		{
			ERROR_EX_LOG("memory alloc failed");
			return;
		}
		p->set_handler(func);
		p->set_code(code);
		p->fill(buf, size);
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
		whttp_msg *p = m_msgs->dequeue();

		while (p)
		{
			p->get_handler()(p->get_code(), p->get_buf(), p->get_size());
			whttp_msg *tmp_msg = p;
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
