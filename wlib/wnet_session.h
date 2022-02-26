/**
*	network
*
*	Copyright (C) 2016 Wang
*
*	Author: H.B. Wang
*	Date:	Aug, 2016
*/

#ifndef W_NET_SESSION_H
#define W_NET_SESSION_H

#include <mutex>
#include <atomic>
#include "wnet_type.h"
#include "wnet_define.h"

namespace wang {

	class wnet_mgr;
	class wnet_msg;

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
		wnet_session(boost::asio::io_service& io_service, wnet_mgr& net_mgr, uint16 index);
		~wnet_session();

		/**
		* 申请资源 单线程
		* @param index	连接 索引号
		*/
		bool init(uint32 send_buff_size, uint32 recv_buff_size);

		/**
		* 释放连接申请的内存资源，单线程
		*/
		void destroy();

		bool is_status_init() const { return m_status == ENSS_Init; }

		// 开始监听 由 Init 状态 变为 Accept
		void set_status_accept();

		// 主动连接 由 Init 状态变为 ConnectTo
		void set_status_connect();

		// 监听或连接失败 由 Accept 或 ConnectTo 状态 变为Init
		void set_status_init();

		/**
		* 服务端打开连接, 移动语义，单线程
		* @param socket	已连接的socket
		*/
		void connected(wnet_msg* msg_ptr);

		// 返回到init后状态, 准备重用. 单线程
		void reset();

		/**
		* 向指定连接发送数据, 多线程. 内部会建立一份拷贝，此函数返回后msg_ptr指向的内存可以安全释放
		* @param session_id	连接id
		* @param msg_ptr	消息
		*/
		bool send_msg(uint32 session_id, wnet_msg* msg_ptr);

		/**
		* 向指定连接发送数据, 多线程. 内部会建立一份拷贝，此函数返回后msg_ptr指向的内存可以安全释放
		* @param session_id	连接id
		* @param msg_ptr		消息的地址
		* @param size			消息的长度
		*/
		bool send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size);

		/**
		* 关闭指定连接, 多线程
		* @param session_id 连接id
		*/
		void close(uint32 session_id);

		/**
		* 单线程
		*/
		uint32 get_id() const { return m_session_id; }

	public:
		/**
		* 单线程
		*/
		wsocket_type& get_socket() { return m_socket; }

		// 读数据回调 单线程
		void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
		// 写数据回调 单线程
		void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

	private:
		// 归还资源
		void _release();

		// 关闭连接
		void _close();

		bool _push_to_queue(const void* msg_ptr, uint32 msg_size, uint16 msg_id);
		void _push_to_queue(wnet_msg* msg_ptr);
		void _queue_to_buf();
		bool _parse_msg();
		void _free_wait_sending();

	private:
		void _swap_send_data_buf();
		void _post_write();

	private:
		wnet_session(const wnet_session& rht);
		wnet_session& operator=(const wnet_session& rht);

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

} // namespace wang

#endif // W_NET_SESSION_H
