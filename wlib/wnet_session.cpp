#include "wnet_session.h"
#include "wlog.h"
#include "wnet_mgr.h"
#include "wnet_msg.h"
#include "wnet_msg_reserve.h"
#include "wmsg.h"
#include <boost/bind.hpp>

namespace wang
{

	wnet_session::wnet_session(boost::asio::io_service& io_service
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

	bool wnet_session::init(uint32 send_buff_size, uint32 recv_buff_size)
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

	void wnet_session::destroy()
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

	void wnet_session::set_status_accept()
	{
		assert(m_status == ENSS_Init);
		m_status = ENSS_Accept;
	}
	void wnet_session::set_status_connect()
	{
		assert(m_status == ENSS_Init);
		m_status = ENSS_ConnectTo;
	}
	void wnet_session::set_status_init()
	{
		assert(m_status == ENSS_Accept || m_status == ENSS_ConnectTo);
		m_status = ENSS_Init;
	}

	void wnet_session::connected(wnet_msg* msg_ptr)
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

	void wnet_session::reset()
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

	bool wnet_session::send_msg(uint32 session_id, wnet_msg* msg_ptr)
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

	bool wnet_session::send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size)
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

	void wnet_session::close(uint32 session_id)
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

	void wnet_session::_release()
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

	void wnet_session::_close()
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

	void wnet_session::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
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

	void wnet_session::handle_write(const boost::system::error_code& error, std::size_t bytes_transferred)
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

	bool wnet_session::_push_to_queue(const void* msg_ptr, uint32 msg_size, uint16 msg_id)
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

	void wnet_session::_push_to_queue(wnet_msg* msg_ptr)
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
	void wnet_session::_queue_to_buf()
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

	bool wnet_session::_parse_msg()
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

	void wnet_session::_free_wait_sending()
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

	void wnet_session::_swap_send_data_buf()
	{
		std::swap(m_send_buf_ptr, m_data_buf_ptr);
		m_send_buf_size = m_data_buf_size;
		m_data_buf_size = 0;
	}
	void wnet_session::_post_write()
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
} // namespace wang
