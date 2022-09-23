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
		//���ӻص�
		typedef std::function<void(uint32 session_id, uint32 para, const char* buf)> wconnect_cb;
		//�Ͽ����ӻص�
		typedef std::function<void(uint32 session_id)> wdisconnect_cb;
		//��Ϣ�ص�1 ����������Ϣ�ṹ
		typedef std::function<void(uint32 session_id, wmsg& msg_ptr)> wmsg_cb;
		//��֤ʧ�ܻص�
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

		// ����������
		bool startup(uint32 thread_num, const std::string& ip, uint16 port);

		// �رռ���
		void shut_accept();

		// �رշ�����
		void shutdown();

		//����
		void destroy();

	public:
		//��������Ϣ��С
		void set_max_msg_size(uint32 value) { m_max_received_msg_size = value; }

		//�Զ�����ʱ��
		void set_reconnet_second(uint32 seconds) { m_reconnect_second = seconds; }

		//��������Ϣ��С
		void set_msg_post_placeholder(uint32 value) { m_msg_post_placeholder = value; }

	public:
		//���ӻص�
		void set_connect_callback(wconnect_cb callback) { m_connect_callback = callback; }

		//�Ͽ����ӻص�
		void set_disconnect_callback(wdisconnect_cb callback) { m_disconnect_callback = callback; }

		//��Ϣ�ص�
		void set_msg_callback(wmsg_cb callback) { m_msg_callback = callback; }

	public:
		/**
		* �첽�����ӷ�����
		* @param ip_addresss	Զ��ip��ַ
		* @param port			Զ�̶˿�
		* @return				true���ӳɹ�,false��ʾ��������Ͷ��ʧ��
		*/
		bool connect_to(uint32 index, const std::string& ip_address, uint16 port);

		// ������Ϣ
		bool send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size);

		// ת����Ϣ
		bool transfer_msg(uint32 session_id, wmsg& msg);

		/**
		* �ر�ָ������
		* @param session_id	����id
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
		//������Ϣ���ص�, �ص�wmsg_cb�Ժ��ͷ���Ϣ
		void process_msg();
		void garbage_collect();
		std::string get_info();
		void show_info();

	private:
		//���� wnet_session ʹ��
		wnet_msg* create_msg(uint16 msg_id, uint32 msg_size);
		void push_msg(wnet_msg* msg_ptr);
		void static_msg(uint16 msg_id, uint32 msg_size);
		void static_msg(wnet_msg* msg_ptr);
		void post_disconnect(wnet_session* session_ptr);

	private:
		// ȡ��һ�����õĻỰ
		wnet_session* _get_available_session();

		// �黹һ���رյĻỰ
		void _return_session(wnet_session* session_ptr);

	private:
		// �����߳�
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

		wservice		m_io_service;		// io����
		wwork			m_work;				// ��֤io����work

		wsessions		m_sessions;

		wsessions		m_available_sessions;
		wlock_type		m_avail_session_mutex;

		wthreads		m_threads;			// �̳߳�

											//server
		wacceptor*		m_acceptor_ptr;		// ����socket
		watomic_uint	m_listening_num;	// ��ǰ����������
		uint32			m_listening_max;	// ������������

											//client
		uint32			m_reconnect_second;
		wparas			m_paras;

		wnet_msg_queue*	m_msgs;
	};

}
#endif

