#ifndef	W_NET_MGR_H
#define W_NET_MGR_H

#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <atomic>
#include "wnet_type.h"
#include "wnet_define.h"

namespace wang
{
	class wmem_pool;
	class wnet_session;
	class wnet_msg;
	class wmsg;
	class wnet_msg_queue;

	class wnet_mgr
	{
		friend class wnet_session;

	public:
		//连接回调
		typedef std::function<void(uint32 session_id, uint32 para, const char* buf)> wconnect_cb;
		//断开连接回调
		typedef std::function<void(uint32 session_id)> wdisconnect_cb;
		//消息回调1 隐藏网络消息结构
		typedef std::function<void(uint32 session_id, wmsg& msg_ptr)> wmsg_cb;
		//验证失败回调
		typedef std::function<void()> wfailed_cb;

	private:
		typedef std::vector<wnet_session*> wsessions;
		typedef std::vector<std::thread> wthreads;

		typedef std::atomic_uint watomic_uint;
		typedef std::atomic_bool watomic_bool;

		typedef std::mutex wlock_type;
		typedef std::lock_guard<wlock_type> wlock_guard;

		typedef boost::asio::io_service wservice;
		typedef boost::asio::io_service::work wwork;
		typedef boost::asio::ip::tcp::acceptor wacceptor;
		typedef boost::asio::ip::tcp::socket wsocket;
		typedef boost::asio::ip::tcp::endpoint wendpoint;
		typedef boost::asio::deadline_timer wtimer;
		typedef boost::system::error_code werror;

	private:
		//client
		struct wconnect_para
		{
			wconnect_para(wservice& io_service);
			std::string		ip;
			uint16			port;
			wtimer			timer;
		};

		typedef std::vector<wconnect_para*> wparas;

	public:
		wnet_mgr();
		~wnet_mgr();

	public:
		static wnet_mgr* construct();
		static void destroy(wnet_mgr* p);

	public:
		bool init(const std::string& name, uint32 client_session, uint32 server_session
			, uint32 send_buf_size, uint32 recv_buf_size, uint32 pool_size
			, wfailed_cb failed_cb, const std::string& auth_name
			, const std::string& auth_pass, uint32 auth_delay);

		// 启动服务器
		bool startup(uint32 thread_num, const std::string& ip, uint16 port);

		// 关闭监听
		void shut_accept();

		// 关闭服务器
		void shutdown();

		//销毁
		void destroy();

	public:
		//最大接收消息大小
		void set_max_msg_size(uint32 value) { m_max_received_msg_size = value; }

		//自动重连时间
		void set_reconnet_second(uint32 seconds) { m_reconnect_second = seconds; }

		//最大接收消息大小
		void set_msg_post_placeholder(uint32 value) { m_msg_post_placeholder = value; }

	public:
		//连接回调
		void set_connect_callback(wconnect_cb callback) { m_connect_callback = callback; }

		//断开连接回调
		void set_disconnect_callback(wdisconnect_cb callback) { m_disconnect_callback = callback; }

		//消息回调
		void set_msg_callback(wmsg_cb callback) { m_msg_callback = callback; }

	public:
		/**
		* 异步的连接服务器
		* @param ip_addresss	远程ip地址
		* @param port			远程端口
		* @return				true连接成功,false表示连接请求投递失败
		*/
		bool connect_to(uint32 index, const std::string& ip_address, uint16 port);

		// 发送消息
		bool send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size);

		// 转发消息
		bool transfer_msg(uint32 session_id, wmsg& msg);

		/**
		* 关闭指定连接
		* @param session_id	连接id
		*/
		void close(uint32 session_id);

	public:
		uint32 get_max_msg_size() const { return m_max_received_msg_size; }
		uint32 get_send_buf_size() const { return m_send_buf_size; }
		uint32 get_recv_buf_size() const { return m_recv_buf_size; }
		uint32 get_reconnet_second() const { return m_reconnect_second; }
		const std::string& get_name() const { return m_name; }
		bool is_server_session(uint32 id) const;

	public:
		//遍历消息并回调, 回调wmsg_cb以后释放消息
		void process_msg();
		void garbage_collect();
		std::string get_info();
		void show_info();

	private:
		//仅供 wnet_session 使用
		wnet_msg* create_msg(uint16 msg_id, uint32 msg_size);
		void push_msg(wnet_msg* msg_ptr);
		void static_msg(uint16 msg_id, uint32 msg_size);
		void static_msg(wnet_msg* msg_ptr);
		void post_disconnect(wnet_session* session_ptr);

	private:
		// 取得一个可用的会话
		wnet_session* _get_available_session();

		// 归还一个关闭的会话
		void _return_session(wnet_session* session_ptr);

	private:
		// 工作线程
		void _worker_thread();

	private:
		//server
		bool _start_listen(const std::string& ip, uint16 port, uint32 thread_num);
		void _post_accept();
		void _handle_accept(const werror& error, wnet_session* session_ptr);
		wnet_msg* _create_accept_msg(const wsocket& socket);

	private:
		//client
		void _handle_connect(const werror& error, uint32 index);
		void _post_reconnect(wconnect_para* para_ptr, uint32 index, uint32 seconds);
		void _handle_reconnect(uint32 index);
		wnet_msg* _create_connect_msg(uint32 error_code);

	private:
		//share
		void _handle_close(wnet_session* session_ptr);
		void _close_impl(uint32 session_id);
		bool _is_server_index(uint32 index) const;

	private:
		wnet_mgr(const wnet_mgr&);
		wnet_mgr& operator=(const wnet_mgr&);

	private:
		std::string		m_name;
		watomic_bool	m_shuting;

		//config
		uint32			m_max_received_msg_size;
		uint32			m_msg_post_placeholder;
		uint32			m_send_buf_size;
		uint32			m_recv_buf_size;

		//callback
		wconnect_cb		m_connect_callback;
		wdisconnect_cb	m_disconnect_callback;
		wmsg_cb			m_msg_callback;

		wmem_pool*		m_pool_ptr;

		wservice		m_io_service;		// io服务
		wwork			m_work;				// 保证io服务work

		wsessions		m_sessions;

		wsessions		m_available_sessions;
		wlock_type		m_avail_session_mutex;

		wthreads		m_threads;			// 线程池

											//server
		wacceptor*		m_acceptor_ptr;		// 监听socket
		watomic_uint	m_listening_num;	// 当前监听的数量
		uint32			m_listening_max;	// 最大监听的数量

											//client
		uint32			m_reconnect_second;
		wparas			m_paras;

		wnet_msg_queue*	m_msgs;
	};

}
#endif

