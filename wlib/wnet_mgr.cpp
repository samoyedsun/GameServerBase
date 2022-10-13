#include "wnet_mgr.h"
#include "wmem_pool.h"
#include "wlog.h"
#include "wnet_func.h"
#include "wmsg.h"
#include "wnet_msg_statistic.h"
#include <boost/bind.hpp>
#include <sstream>

namespace wang
{
	enum EMsgIDReserve
	{
		EMIR_Connect = 1, //建立连接
		EMIR_Disconnect,

		EMIR_Max,
	};

	/**
	* 监听建立连接 或 主动建立连接
	* buf[0] == 0 表示主动建立连接, 否则表示监听建立连接
	* 监听建立连接 para表示buf长度, buf表示ip地址
	* 主动建立连接 para表示错误码,  buf[0] == 0
	*/
	struct wmsg_connect
	{
		uint32 para;
		char buf[1];
	};


#pragma pack(push,1)

	class wnet_msg
	{
	private:
		explicit wnet_msg(wmem_pool* pool_ptr, uint16 msg_id, uint32 msg_size)
			: m_pool_ptr(pool_ptr)
			, m_next_ptr(nullptr)
			, m_session_id(0)
			, m_reference(1)
			, m_msg(msg_id, msg_size)
		{
		}

		~wnet_msg()
		{
		}

	public:
		static wnet_msg* alloc_me(wmem_pool* pool_ptr, uint16 msg_id, uint32 msg_size
			, uint32 placeholder = 0)
		{
			void* p = pool_ptr->alloc(sizeof(wnet_msg) - 1 + msg_size + placeholder);
			if (!p)
			{
				return nullptr;
			}
			wnet_msg* q = new (p) wnet_msg(pool_ptr, msg_id, msg_size);
			return q;
		}

		void free_me()
		{
			this->~wnet_msg();
			m_pool_ptr->free(this);
		}

	public:
		wnet_msg* get_next() { return m_next_ptr; }
		void set_next(wnet_msg* value) { m_next_ptr = value; }

		uint32 get_session_id() const { return m_session_id; }
		void set_session_id(uint32 value) { m_session_id = value; }

		void set_reference(uint16 value)
		{
			m_reference.store(value);
		}

		void sub_reference()
		{
			if (m_reference.fetch_sub(1) == 1)
			{
				free_me();
			}
		}
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

		//p指向m_buffer
		static wnet_msg* get_net_msg(wmsg* p)
		{
			return reinterpret_cast<wnet_msg*>(
				reinterpret_cast<char*>(p) - (sizeof(wnet_msg) - sizeof(wmsg))
				);
		}

	//private:
	//	wnet_msg(const wnet_msg&);
	//	wnet_msg& operator=(const wnet_msg&);

	private:
		wmem_pool*			m_pool_ptr;
		wnet_msg*			m_next_ptr;
		uint32				m_session_id;
		std::atomic_ushort	m_reference;
		wmsg				m_msg;
	};

#pragma pack(pop)

	class wnet_session
	{
	public:
		typedef std::mutex wlock_type;
		typedef std::lock_guard<wlock_type> wguard_type;
		typedef boost::asio::ip::tcp::socket wsocket_type;

	private:
		/**
		*	网络底层session状态
		*/
		enum wENetSessionStatus
		{
			ENSS_None = 0,
			ENSS_Init,
			ENSS_Accept,
			ENSS_ConnectTo,
			ENSS_Open,
			ENSS_Shut,
			ENSS_Close,
		};

	public:
		wnet_session(boost::asio::io_service& io_service
			, wnet_mgr& net_mgr, uint16 index)
			: m_session_id(0X00010000 | index)
			, m_socket(io_service)
			, m_net_mgr(net_mgr)
			, m_status(ENSS_None)
			, m_is_writing(false)
			, m_is_reading(false)
			, m_recv_buf_ptr(nullptr)
			, m_recv_msg_ptr(nullptr)
			, m_recv_size(0)
			, m_data_buf_ptr(nullptr)
			, m_data_buf_size(0)
			, m_send_buf_ptr(nullptr)
			, m_send_buf_size(0)
			, m_send_size(0)
			, m_wait_head_ptr(nullptr)
			, m_wait_tail_ptr(nullptr)
		{

		}
		wnet_session::~wnet_session()
		{

		}

		/**
		* 申请资源 单线程
		* @param index	连接 索引号
		*/
		bool init(uint32 send_buff_size, uint32 recv_buff_size)
		{
			// alloc memory
			if (recv_buff_size < MSG_HEADER_SIZE)
			{
				LOG_ERROR << "recv_buff_size too small, recv_buff_size=" << recv_buff_size;
				return false;
			}
			m_recv_buf_ptr = new char[recv_buff_size];
			if (!m_recv_buf_ptr)
			{
				LOG_ERROR << "alloc m_recv_buf_ptr memory error!";
				return false;
			}

			m_data_buf_ptr = new char[send_buff_size];
			if (!m_data_buf_ptr)
			{
				return false;
			}

			m_send_buf_ptr = new char[send_buff_size];
			if (!m_send_buf_ptr)
			{
				return false;
			}

			m_status = ENSS_Init;
			return true;
		}

		/**
		* 释放连接申请的内存资源，单线程
		*/
		void destroy()
		{
			m_status = ENSS_None;

			if (m_recv_buf_ptr)
			{
				delete[] m_recv_buf_ptr;
				m_recv_buf_ptr = nullptr;
			}

			if (m_recv_msg_ptr)
			{
				m_recv_msg_ptr->free_me();
				m_recv_msg_ptr = nullptr;
			}

			if (m_data_buf_ptr)
			{
				delete[] m_data_buf_ptr;
				m_data_buf_ptr = nullptr;
			}
			if (m_send_buf_ptr)
			{
				delete[] m_send_buf_ptr;
				m_send_buf_ptr = nullptr;
			}

			if (m_wait_head_ptr)
			{
				_free_wait_sending();
			}
		}

		bool is_status_init() const
		{
			return m_status == ENSS_Init;
		}

		// 开始监听 由 Init 状态 变为 Accept
		void set_status_accept()
		{
			assert(m_status == ENSS_Init);
			m_status = ENSS_Accept;
		}

		// 主动连接 由 Init 状态变为 ConnectTo
		void set_status_connect()
		{
			assert(m_status == ENSS_Init);
			m_status = ENSS_ConnectTo;
		}

		// 监听或连接失败 由 Accept 或 ConnectTo 状态 变为Init
		void set_status_init()
		{
			assert(m_status == ENSS_Accept || m_status == ENSS_ConnectTo);
			m_status = ENSS_Init;
		}

		/**
		* 服务端打开连接, 移动语义，单线程
		* @param socket	已连接的socket
		*/
		void connected(wnet_msg* msg_ptr)
		{
			assert(m_status == ENSS_Accept || m_status == ENSS_ConnectTo);

			//m_socket = std::move(socket);
			m_status = ENSS_Open;
			m_is_reading.store(true);

			//回调消息
			msg_ptr->set_session_id(m_session_id);
			m_net_mgr.push_msg(msg_ptr);

			// 投递读请求
			m_socket.async_read_some(boost::asio::buffer(m_recv_buf_ptr, m_net_mgr.get_recv_buf_size()),
				boost::bind(&wnet_session::handle_read, this, boost::asio::placeholders::error
					, boost::asio::placeholders::bytes_transferred)
			);
		}

		// 返回到init后状态, 准备重用. 单线程
		void reset()
		{
			assert(m_status == ENSS_Close);

			if ((m_session_id & 0XFFFF0000) == 0XFFFF0000)
			{
				m_session_id += 0X00020000;
			}
			else
			{
				m_session_id += 0X00010000;
			}

			m_is_writing = false;
			m_is_reading = false;

			m_recv_size = 0;

			if (m_recv_msg_ptr)
			{
				m_recv_msg_ptr->free_me();
				m_recv_msg_ptr = NULL;
			}

			m_data_buf_size = 0;

			m_send_buf_size = 0;

			m_send_size = 0;

			if (m_wait_head_ptr)
			{
				_free_wait_sending();
			}

			m_status = ENSS_Init;
		}

		/**
		* 向指定连接发送数据, 多线程. 内部会建立一份拷贝，此函数返回后msg_ptr指向的内存可以安全释放
		* @param session_id	连接id
		* @param msg_ptr	消息
		*/
		bool send_msg(uint32 session_id, wnet_msg* msg_ptr)
		{
			// 检查参数及连接的有效性,m_session_id和m_status多线程访问
			if (!msg_ptr || session_id != m_session_id || 0 == msg_ptr->get_msg_id() || ENSS_Open != m_status)
			{
				return false;
			}

			m_net_mgr.static_msg(msg_ptr);

			{
				// 获得锁
				wguard_type lock(m_mutex);

				if (session_id != m_session_id || ENSS_Open != m_status)
				{
					return false;
				}

				if (m_wait_head_ptr)
				{
					_push_to_queue(msg_ptr);
					msg_ptr->add_reference();
					return true;
				}

				if (msg_ptr->get_msg_size() <= m_net_mgr.get_send_buf_size() - m_data_buf_size)
				{
					//可以放下整个消息
					memcpy(m_data_buf_ptr + m_data_buf_size, msg_ptr->get_msg_buf(), msg_ptr->get_msg_size());
					m_data_buf_size += msg_ptr->get_msg_size();
				}
				else
				{
					_push_to_queue(msg_ptr);
					msg_ptr->add_reference();
				}

				bool is_writing = false;
				if (!m_is_writing.compare_exchange_strong(is_writing, true))
				{
					return true;
				}

				_swap_send_data_buf();

				// 投递写请求
				_post_write();
			}

			return true;
		}

		/**
		* 向指定连接发送数据, 多线程. 内部会建立一份拷贝，此函数返回后msg_ptr指向的内存可以安全释放
		* @param session_id	连接id
		* @param msg_ptr		消息的地址
		* @param size			消息的长度
		*/
		bool send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size)
		{
			// 检查参数及连接的有效性,m_session_id和m_status多线程访问
			if (session_id != m_session_id || 0 == msg_id || ENSS_Open != m_status)
			{
				return false;
			}

			m_net_mgr.static_msg(msg_id, size);


			{// 括号限制m_mutex保护的范围，避免递归
			 // 获得锁
				wguard_type lock(m_mutex);

				if (session_id != m_session_id || ENSS_Open != m_status)
				{
					return false;
				}

				if (m_wait_head_ptr)
				{
					return _push_to_queue(msg_ptr, size, msg_id);
				}

				if (MSG_HEADER_SIZE + size <= m_net_mgr.get_send_buf_size() - m_data_buf_size)
				{
					//可以放下整个消息
					memcpy(m_data_buf_ptr + m_data_buf_size, &msg_id, sizeof(msg_id));
					m_data_buf_size += sizeof(msg_id);

					memcpy(m_data_buf_ptr + m_data_buf_size, &size, sizeof(size));
					m_data_buf_size += sizeof(size);

					memcpy(m_data_buf_ptr + m_data_buf_size, msg_ptr, size);
					m_data_buf_size += size;
				}
				else if (!_push_to_queue(msg_ptr, size, msg_id))
				{
					return false;
				}

				bool is_writing = false;
				if (!m_is_writing.compare_exchange_strong(is_writing, true))
				{
					return true;
				}

				_swap_send_data_buf();

				// 投递写请求
				_post_write();
			}

			return true;
		}

		/**
		* 关闭指定连接, 多线程
		* @param session_id 连接id
		*/
		void close(uint32 session_id)
		{
			// 检查参数及连接的有效性,m_session_id和m_status多线程访问
			if (session_id != m_session_id || ENSS_Open != m_status)
			{
				return;
			}

			{// 括号保护m_mutex访问避免递归，helper析构里可能访问m_mutex
				wguard_type guard(m_mutex);

				if (session_id != m_session_id || ENSS_Open != m_status)
				{
					return;
				}

				m_status = ENSS_Shut;

				boost::system::error_code ec;
				m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
				m_socket.close(ec);
			}

			// 尝试释放资源
			_release();
		}

		/**
		* 单线程
		*/
		uint32 get_id() const
		{
			return m_session_id;
		}

	public:
		/**
		* 单线程
		*/
		wsocket_type& get_socket()
		{
			return m_socket;
		}

		// 读数据回调 单线程
		void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
		{
			if (error || 0 == bytes_transferred)
			{
				m_is_reading.store(false);
				_close();
				return;
			}

			// 可能外部调用了close
			if (ENSS_Open != m_status)
			{
				m_is_reading.store(false);
				_release();
				return;
			}

			m_recv_size += static_cast<uint32>(bytes_transferred);
			if (m_recv_msg_ptr)
			{
				//已经构造了消息, 在消息体里接收数据
				if (m_recv_size == m_recv_msg_ptr->get_size())
				{
					//完整消息
					m_net_mgr.static_msg(m_recv_msg_ptr);
					m_net_mgr.push_msg(m_recv_msg_ptr);
					m_recv_msg_ptr = NULL;
					m_recv_size = 0;
				}
				else if (m_recv_size > m_recv_msg_ptr->get_size())
				{
					LOG_ERROR << "recv size too big, recv size=" << m_recv_size
						<< ", expect_size=" << m_recv_msg_ptr->get_size()
						<< ", name=" << m_net_mgr.get_name();
					m_is_reading.store(false);
					_close();
					return;
				}
				else
				{
					LOG_WARN << "read not all, name=" << m_net_mgr.get_name();
				}
			}
			else if (m_recv_size >= MSG_HEADER_SIZE)
			{
				//解析消息
				if (!_parse_msg())
				{
					m_is_reading.store(false);
					_close();
					return;
				}
			}

			if (m_recv_msg_ptr)
			{
				//read msg body
				assert(m_recv_msg_ptr->get_size() > m_recv_size);
				wguard_type lock(m_mutex);
				boost::asio::async_read(m_socket,
					//m_socket.async_read_some(
					boost::asio::buffer(m_recv_msg_ptr->get_buf() + m_recv_size
						, m_recv_msg_ptr->get_size() - m_recv_size),
					boost::bind(&wnet_session::handle_read, this, boost::asio::placeholders::error
						, boost::asio::placeholders::bytes_transferred));
			}
			else
			{
				//msg header not ready, continue read msg header
				assert(m_net_mgr.get_recv_buf_size() > m_recv_size);
				wguard_type lock(m_mutex);
				m_socket.async_read_some(
					boost::asio::buffer(m_recv_buf_ptr + m_recv_size
						, m_net_mgr.get_recv_buf_size() - m_recv_size),
					boost::bind(&wnet_session::handle_read, this, boost::asio::placeholders::error
						, boost::asio::placeholders::bytes_transferred)
				);
			}
		}

		// 写数据回调 单线程
		void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred)
		{
			if (error || 0 == bytes_transferred)
			{
				m_is_writing.store(false);
				_close();
				return;
			}

			if (ENSS_Open != m_status)
			{
				m_is_writing.store(false);
				_release();
				return;
			}

			m_send_size += static_cast<uint32>(bytes_transferred);

			if (m_send_buf_size > 0)
			{
				//发送的缓冲区
				if (m_send_size == m_send_buf_size)
				{
					m_send_size = 0;
					m_send_buf_size = 0;
					{
						wguard_type lock(m_mutex);
						if (m_data_buf_size > 0)
						{
							_swap_send_data_buf();
							_post_write();
							return;
						}
						else if (m_wait_head_ptr)
						{
							//尽量打包
							_queue_to_buf();
							_post_write();
							return;
						}
					}

					m_is_writing.store(false);
					if (ENSS_Open != m_status)
					{
						_release();
					}
					return;
				}
			}
			else
			{
				//发送的消息
				if (m_send_size == m_wait_head_ptr->get_msg_size())
				{
					//一个消息发送完
					m_send_size = 0;
					{
						wguard_type lock(m_mutex);
						wnet_msg* q = m_wait_head_ptr;
						m_wait_head_ptr = m_wait_head_ptr->get_next();
						q->sub_reference();
						if (m_wait_head_ptr)
						{
							//尽量打包
							_queue_to_buf();
							_post_write();
							return;
						}
						else
						{
							//没有消息了
							m_wait_tail_ptr = NULL;
						}
					}

					m_is_writing.store(false);
					if (ENSS_Open != m_status)
					{
						_release();
					}
					return;
				}
			}

			LOG_WARN << "write not all, name=" << m_net_mgr.get_name();

			{
				// 投递写请求
				wguard_type lock(m_mutex);
				_post_write();
			}
		}

	private:
		// 归还资源
		void _release()
		{
			wguard_type guard(m_mutex);

			if (ENSS_Shut != m_status || m_is_writing || m_is_reading)
			{
				return;
			}

			// 设置session状态
			m_status = ENSS_Close;

			m_net_mgr.post_disconnect(this);
		}

		// 关闭连接
		void _close()
		{
			{// 括号保护m_mutex访问避免递归，helper析构里可能访问m_mutex
				wguard_type guard(m_mutex);

				if (ENSS_Open == m_status)
				{
					m_status = ENSS_Shut;

					boost::system::error_code ec;
					m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
					m_socket.close(ec);
				}
			}

			// 尝试释放资源
			_release();
		}

		bool _push_to_queue(const void* msg_ptr, uint32 msg_size, uint16 msg_id)
		{
			wnet_msg* p = m_net_mgr.create_msg(msg_id, msg_size);
			if (!p)
			{
				LOG_ERROR << "alloc failed, name=" << m_net_mgr.get_name();
				return false;
			}

			p->fill(msg_ptr, msg_size);

			_push_to_queue(p);

			return true;
		}

		void _push_to_queue(wnet_msg* msg_ptr)
		{
			if (!m_wait_tail_ptr)
			{
				m_wait_head_ptr = m_wait_tail_ptr = msg_ptr;
			}
			else
			{
				m_wait_tail_ptr->set_next(msg_ptr);
				m_wait_tail_ptr = msg_ptr;
			}
		}

		void _queue_to_buf()
		{
			//小消息打包, 大消息直接发送
			uint32 msg_size = m_wait_head_ptr->get_msg_size();
			while (msg_size <= m_net_mgr.get_send_buf_size() - m_send_buf_size)
			{
				memcpy(m_send_buf_ptr + m_send_buf_size, m_wait_head_ptr->get_msg_buf(), msg_size);
				m_send_buf_size += msg_size;

				if (m_wait_head_ptr->get_next())
				{
					wnet_msg* q = m_wait_head_ptr;
					m_wait_head_ptr = m_wait_head_ptr->get_next();
					q->sub_reference();
					msg_size = m_wait_head_ptr->get_msg_size();
				}
				else
				{
					m_wait_head_ptr->sub_reference();
					m_wait_head_ptr = NULL;
					m_wait_tail_ptr = NULL;
					break;
				}
			};
		}

		bool _parse_msg()
		{
			const char* buf = m_recv_buf_ptr;
			while (true)
			{
				const wmsg* header_ptr = reinterpret_cast<const wmsg*>(buf);
				if (header_ptr->get_id() < EMIR_Max)
				{
					LOG_ERROR << "invalid msg id, msg_id=" << header_ptr->get_id()
						<< ", name=" << m_net_mgr.get_name();
					return false;
				}
				if (header_ptr->get_size() > m_net_mgr.get_max_msg_size())
				{
					LOG_ERROR << "msg size too big, msg_id=" << header_ptr->get_id()
						<< ", msg_size=" << header_ptr->get_size()
						<< ", max_size =" << m_net_mgr.get_max_msg_size()
						<< ", name=" << m_net_mgr.get_name();
					return false;
				}
				//new msg
				m_recv_msg_ptr = m_net_mgr.create_msg(header_ptr->get_id(), header_ptr->get_size());
				if (!m_recv_msg_ptr)
				{
					LOG_ERROR << "alloc msg failed, msg_id=" << header_ptr->get_id()
						<< ", msg_size=" << header_ptr->get_size()
						<< ", name=" << m_net_mgr.get_name();
					return false;
				}

				m_recv_msg_ptr->set_session_id(m_session_id);

				buf += MSG_HEADER_SIZE;
				m_recv_size -= MSG_HEADER_SIZE;
				if (m_recv_size >= header_ptr->get_size())
				{
					//一个完成消息
					if (header_ptr->get_size() > 0)
					{
						m_recv_msg_ptr->fill(buf, header_ptr->get_size());
						buf += header_ptr->get_size();
						m_recv_size -= header_ptr->get_size();
					}

					m_net_mgr.static_msg(m_recv_msg_ptr);
					m_net_mgr.push_msg(m_recv_msg_ptr);
					m_recv_msg_ptr = NULL;

					if (m_recv_size < MSG_HEADER_SIZE)
					{
						//已不足一个消息头
						memmove(m_recv_buf_ptr, buf, m_recv_size);
						return true;
					}
				}
				else
				{
					// 不足一个消息
					m_recv_msg_ptr->fill(buf, m_recv_size);
					return true;
				}
			};
			return true;
		}

		void _free_wait_sending()
		{
			wnet_msg* p = m_wait_head_ptr;
			m_wait_head_ptr = NULL;
			m_wait_tail_ptr = NULL;
			while (p)
			{
				wnet_msg* q = p;
				p = p->get_next();
				q->free_me();
			}
		}

	private:
		void _swap_send_data_buf()
		{
			std::swap(m_send_buf_ptr, m_data_buf_ptr);
			m_send_buf_size = m_data_buf_size;
			m_data_buf_size = 0;
		}

		void _post_write()
		{
			if (m_send_buf_size > 0)
			{
				boost::asio::async_write(m_socket,
					//m_socket.async_write_some(
					boost::asio::buffer(m_send_buf_ptr + m_send_size, m_send_buf_size - m_send_size),
					boost::bind(&wnet_session::handle_write, this, boost::asio::placeholders::error
						, boost::asio::placeholders::bytes_transferred));
			}
			else
			{
				boost::asio::async_write(m_socket,
					//m_socket.async_write_some(
					boost::asio::buffer(static_cast<const char *>(m_wait_head_ptr->get_msg_buf()) + m_send_size
						, m_wait_head_ptr->get_msg_size() - m_send_size),
					boost::bind(&wnet_session::handle_write, this, boost::asio::placeholders::error
						, boost::asio::placeholders::bytes_transferred));
			}
		}

	//private:
	//	wnet_session(const wnet_session& rht);
	//	wnet_session& operator=(const wnet_session& rht);

	private:
		uint32				m_session_id;		// session_id一个连接

		wsocket_type		m_socket;			// session的socket资源

		wnet_mgr&			m_net_mgr;

		wlock_type			m_mutex;			// 保护send缓冲区
		wENetSessionStatus	m_status;			// session状态
		std::atomic<bool>	m_is_writing;		// 已投递异步write还没有返回
		std::atomic<bool>	m_is_reading;		//

												//receive msg
		char*				m_recv_buf_ptr;		// 接收消息
		wnet_msg*			m_recv_msg_ptr;
		uint32				m_recv_size;

		//send msg
		char*				m_data_buf_ptr;		// 发送数据缓冲区
		uint32				m_data_buf_size;	// 发送数据缓冲区已填充数据大小

		char*				m_send_buf_ptr;		// 正在发送数据缓冲区
		uint32				m_send_buf_size;	// 正在发送数据缓冲区已填充数据大小
		uint32				m_send_size;		// 正在发送数据缓冲区已发送数据大小

		wnet_msg*			m_wait_head_ptr;	// 等待发送缓冲区头
		wnet_msg*			m_wait_tail_ptr;	// 等待发送缓冲区尾
	};


	class wnet_msg_queue
	{
	public:
		wnet_msg_queue()
			: m_head_ptr(NULL)
			, m_tail_ptr(NULL)
		{
		}
		~wnet_msg_queue()
		{
		}

		void enqueue(wnet_msg* msg_ptr)
		{
			std::lock_guard<std::mutex> guard(m_mutex);

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

		wnet_msg* dequeue()
		{
			std::lock_guard<std::mutex> guard(m_mutex);

			wnet_msg *ptr = m_head_ptr;
			if (ptr)
			{
				m_head_ptr = ptr->get_next();
				if (m_head_ptr == nullptr)
				{
					m_tail_ptr = m_head_ptr;
				}
			}
			return ptr;
		}

		void clear()
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

	private:
		std::mutex	m_mutex;
		wnet_msg*	m_head_ptr;
		wnet_msg*	m_tail_ptr;
	};


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
				p = m_msgs->dequeue();
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
		SYSTEM_LOG("发送消息，session_id:%d, msg_id:%d, size:%d", session_id, msg_id, size);

		const uint32 index = get_session_index(session_id);
		if (index >= m_sessions.size())
		{
			return false;
		}

		return m_sessions[index]->send_msg(session_id, msg_id, msg_ptr, size);
	}

	bool wnet_mgr::transfer_msg(uint32 session_id, wmsg& msg)
	{
		SYSTEM_LOG("转发消息，session_id:%d, msg_id:%d, size:%d", session_id, msg.get_id(), msg.get_size());

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
			msg_ptr = m_msgs->dequeue();
			p->set_next(NULL);
			if (p->get_msg_id() == EMIR_Connect)
			{
				SYSTEM_LOG("收到连接消息，session_id:%d", p->get_session_id());
				wmsg_connect* q = (wmsg_connect*)p->get_buf();
				m_connect_callback(p->get_session_id(), q->para, q->buf);
				p->free_me();
			}
			else if (p->get_msg_id() == EMIR_Disconnect)
			{
				SYSTEM_LOG("收到断连消息，session_id:%d", p->get_session_id());
				m_disconnect_callback(p->get_session_id());
				_close_impl(p->get_session_id());
				p->free_me();
			}
			else
			{
				SYSTEM_LOG("收到普通消息，session_id:%d, msg_id:%d, msg_size:%d",
					p->get_session_id(), p->get_msg().get_id(), p->get_msg().get_size());
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
		SYSTEM_LOG("%s", get_info().c_str());
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

		SYSTEM_LOG("listen on ip=%s, port=%d", ip.c_str(), port);
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