#ifndef W_HTTP_CLIENT_H
#define W_HTTP_CLIENT_H

#include "wnet_type.h"
#include "wnet_define.h"
#include <thread>
#include <mutex>
#include <string>
#include <list>

namespace wang {

	class wmem_pool;
	class whttp_msg_queue;
	class whttp_session;

	/**
	*	whttp_client 并发http请求通信底层
	*/
	class whttp_client
	{
	public:
		typedef std::function<void(int code, const void* buf, uint32 size)> wmsg_handler;

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
		bool init(uint32 max_session, uint32 send_buf_size, uint32 recv_buf_size);

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
		bool http_request(const std::string& ip, const std::string port
			, const char* request_ptr, uint32 request_size
			, wmsg_handler func, uint32 time_out_sec = 10);

		uint32 get_avaliable();

	public:
		void process_msg();
		void garbage_collect();

	private:
		void on_response(wmsg_handler func, whttp_session* session_ptr
			, int code, const void* buf, uint32 size);

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

		wmem_pool*						m_pool_ptr;
		whttp_msg_queue*					m_msgs;
	};

} // namespace wang

#endif // W_HTTP_CLIENT_H
