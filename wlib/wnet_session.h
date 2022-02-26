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
		*	����ײ�session״̬
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
		* ������Դ ���߳�
		* @param index	���� ������
		*/
		bool init(uint32 send_buff_size, uint32 recv_buff_size);

		/**
		* �ͷ�����������ڴ���Դ�����߳�
		*/
		void destroy();

		bool is_status_init() const { return m_status == ENSS_Init; }

		// ��ʼ���� �� Init ״̬ ��Ϊ Accept
		void set_status_accept();

		// �������� �� Init ״̬��Ϊ ConnectTo
		void set_status_connect();

		// ����������ʧ�� �� Accept �� ConnectTo ״̬ ��ΪInit
		void set_status_init();

		/**
		* ����˴�����, �ƶ����壬���߳�
		* @param socket	�����ӵ�socket
		*/
		void connected(wnet_msg* msg_ptr);

		// ���ص�init��״̬, ׼������. ���߳�
		void reset();

		/**
		* ��ָ�����ӷ�������, ���߳�. �ڲ��Ὠ��һ�ݿ������˺������غ�msg_ptrָ����ڴ���԰�ȫ�ͷ�
		* @param session_id	����id
		* @param msg_ptr	��Ϣ
		*/
		bool send_msg(uint32 session_id, wnet_msg* msg_ptr);

		/**
		* ��ָ�����ӷ�������, ���߳�. �ڲ��Ὠ��һ�ݿ������˺������غ�msg_ptrָ����ڴ���԰�ȫ�ͷ�
		* @param session_id	����id
		* @param msg_ptr		��Ϣ�ĵ�ַ
		* @param size			��Ϣ�ĳ���
		*/
		bool send_msg(uint32 session_id, uint16 msg_id, const void* msg_ptr, uint32 size);

		/**
		* �ر�ָ������, ���߳�
		* @param session_id ����id
		*/
		void close(uint32 session_id);

		/**
		* ���߳�
		*/
		uint32 get_id() const { return m_session_id; }

	public:
		/**
		* ���߳�
		*/
		wsocket_type& get_socket() { return m_socket; }

		// �����ݻص� ���߳�
		void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);
		// д���ݻص� ���߳�
		void handle_write(const boost::system::error_code& error, std::size_t bytes_transferred);

	private:
		// �黹��Դ
		void _release();

		// �ر�����
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
		uint32				m_session_id;		// session_idһ������

		wsocket_type		m_socket;			// session��socket��Դ

		wnet_mgr&			m_net_mgr;

		wlock_type			m_mutex;			// ����send������
		wENetSessionStatus	m_status;			// session״̬
		std::atomic<bool>	m_is_writing;		// ��Ͷ���첽write��û�з���
		std::atomic<bool>	m_is_reading;		//

												//receive msg
		char*				m_recv_buf_ptr;		// ������Ϣ
		wnet_msg*			m_recv_msg_ptr;
		uint32				m_recv_size;

		//send msg
		char*				m_data_buf_ptr;		// �������ݻ�����
		uint32				m_data_buf_size;	// �������ݻ�������������ݴ�С

		char*				m_send_buf_ptr;		// ���ڷ������ݻ�����
		uint32				m_send_buf_size;	// ���ڷ������ݻ�������������ݴ�С
		uint32				m_send_size;		// ���ڷ������ݻ������ѷ������ݴ�С

		wnet_msg*			m_wait_head_ptr;	// �ȴ����ͻ�����ͷ
		wnet_msg*			m_wait_tail_ptr;	// �ȴ����ͻ�����β
	};

} // namespace wang

#endif // W_NET_SESSION_H
