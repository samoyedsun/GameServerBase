#ifndef W_HTTP_SESSION_H
#define W_HTTP_SESSION_H

#include "wnet_type.h"
#include "wnet_define.h"
#include <string>

namespace wang {

	/**
	*	whttp_session 是http网络传输会话(基于boost.asio)
	*/
	class whttp_session
	{
	public:
		typedef std::function<void(whttp_session*)> wresponse;

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

} // namespace wang

#endif // _NNETWORK_whttp_session_H_
