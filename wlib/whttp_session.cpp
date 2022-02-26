#include "whttp_session.h"
#include "whttp_client.h"
#include "wlog.h"
#include "whttp_request_helper.h"
#include <boost/bind.hpp>

namespace wang {

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

		_handle_result(EHRCode_Err, err.message());
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
			m_response(this);
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
		if (status_code == EHRCode_Succ)
		{

		}
		else
		{
			*reinterpret_cast<uint32*>(m_recv_buf) = status_code;
			m_recv_size = 4;
			if (error_msg.length() < m_recv_buf_size - 4)
			{
				memcpy(m_recv_buf + 4, error_msg.c_str(), error_msg.length());
				m_recv_size += static_cast<uint32>(error_msg.length());
			}
		}

		// 修改请求计数
		m_is_requesting = false;

		// 关闭连接
		_close();
	}

} // namespace wang
