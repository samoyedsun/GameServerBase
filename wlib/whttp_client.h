#ifndef _W_HTTP_CLIENT_H_
#define _W_HTTP_CLIENT_H_

#include "wnet_type.h"
#include "wnet_define.h"
#include <thread>
#include <mutex>
#include <string>
#include <list>

namespace wang {

	class wmem_pool;
	class wnet_msg_queue;
	class whttp_session;

	// http消息数据
	struct whttp_message
	{
		uint32							client_id;			// 用户id
		boost::asio::streambuf			request;			// 请求缓存数据
		boost::asio::streambuf			response;			// 回复缓存数据
		uint32							status_code;		// 状态码
		std::string						error_msg;			// 错误消息
		uint16							msg_id;				// 回调消息id

		whttp_message() : client_id(0), status_code(0), msg_id(0) {}
	};

	/**
	*	whttp_client 并发http请求通信底层
	*/
	class whttp_client
	{
		friend class whttp_session;

	public:
		typedef std::function<void(uint32 client_id, uint16 msg_id, const void* buf, uint32 size)> wmsg_handler;

	private:
		typedef std::mutex wlock_type;
		typedef std::lock_guard<wlock_type> wlock_guard;
		typedef std::vector<whttp_session*> wsessions;

	public:
		whttp_client();
		~whttp_client();

		/**
		* 启动客户端，初始化资源
		*/
		bool init(wmsg_handler msg_handler, uint32 max_session, uint32 send_buf_size, uint32 recv_buf_size);

		void destroy();

		bool startup(const char* name_ptr);

		/**
		* 关闭客户端，该函数所在执行线程不被强制关闭，建议在主线程中关闭
		*/
		void shutdown();

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
		bool http_request(const std::string& host, const char* request_ptr, uint32 request_size, uint32 client_id, uint16 msg_id, uint32 time_out_sec = 10);

		uint32 get_avaliable();

	public:
		void process_msg();
		void garbage_collect();

	private:
		void on_response(uint32 client_id, uint16 msg_id, whttp_session* session_ptr);

	private:
		// 工作线程
		void _work();

	public:
		// 取得一个可用的会话
		whttp_session* _get_available_session();

		// 归还一个关闭的会话
		void _return_session(whttp_session* session_ptr);

	private:
		whttp_client(const whttp_client&);
		whttp_client& operator=(const whttp_client&);

	private:
		boost::asio::io_service			m_io_service;			// io服务
		boost::asio::io_service::work	m_work;					// 保证io服务work
		bool							m_is_working;			// 服务是否开启

		wlock_type						m_mutex;				// 保护连接池线程安全及状态改变
		wsessions						m_sessions;
		std::list<whttp_session*>		m_available_sessions;

		std::thread						m_thread;
		volatile bool					m_stop;

		uint32							m_max_session;			// 最大连接数

		wmsg_handler					m_msg_handler;

		wmem_pool*						m_pool_ptr;
		wnet_msg_queue*					m_msgs;
	};

} // namespace wang

#endif // _NNETWORK_whttp_client_H_
