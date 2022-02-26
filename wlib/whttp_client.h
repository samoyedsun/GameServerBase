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

	// http��Ϣ����
	struct whttp_message
	{
		uint32							client_id;			// �û�id
		boost::asio::streambuf			request;			// ���󻺴�����
		boost::asio::streambuf			response;			// �ظ���������
		uint32							status_code;		// ״̬��
		std::string						error_msg;			// ������Ϣ
		uint16							msg_id;				// �ص���Ϣid

		whttp_message() : client_id(0), status_code(0), msg_id(0) {}
	};

	/**
	*	whttp_client ����http����ͨ�ŵײ�
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
		* �����ͻ��ˣ���ʼ����Դ
		*/
		bool init(wmsg_handler msg_handler, uint32 max_session, uint32 send_buf_size, uint32 recv_buf_size);

		void destroy();

		bool startup(const char* name_ptr);

		/**
		* �رտͻ��ˣ��ú�������ִ���̲߳���ǿ�ƹرգ����������߳��йر�
		*/
		void shutdown();

		/**
		* �첽����http����
		* @param host			host��ַ
		* @param method		����
		* @param param			����
		* @param client_id		�����ʶid
		* @param msg_id		������Ϣid
		* @param time_out_sec	��ʱʱ�䣬��λs
		* @return				true����ɹ�, false��ʾ����Ͷ��ʧ��
		*/
		bool http_request(const std::string& host, const char* request_ptr, uint32 request_size, uint32 client_id, uint16 msg_id, uint32 time_out_sec = 10);

		uint32 get_avaliable();

	public:
		void process_msg();
		void garbage_collect();

	private:
		void on_response(uint32 client_id, uint16 msg_id, whttp_session* session_ptr);

	private:
		// �����߳�
		void _work();

	public:
		// ȡ��һ�����õĻỰ
		whttp_session* _get_available_session();

		// �黹һ���رյĻỰ
		void _return_session(whttp_session* session_ptr);

	private:
		whttp_client(const whttp_client&);
		whttp_client& operator=(const whttp_client&);

	private:
		boost::asio::io_service			m_io_service;			// io����
		boost::asio::io_service::work	m_work;					// ��֤io����work
		bool							m_is_working;			// �����Ƿ���

		wlock_type						m_mutex;				// �������ӳ��̰߳�ȫ��״̬�ı�
		wsessions						m_sessions;
		std::list<whttp_session*>		m_available_sessions;

		std::thread						m_thread;
		volatile bool					m_stop;

		uint32							m_max_session;			// ���������

		wmsg_handler					m_msg_handler;

		wmem_pool*						m_pool_ptr;
		wnet_msg_queue*					m_msgs;
	};

} // namespace wang

#endif // _NNETWORK_whttp_client_H_
