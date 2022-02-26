#ifndef W_HTTP_SESSION_H
#define W_HTTP_SESSION_H

#include "wnet_type.h"
#include "wnet_define.h"
#include <string>

namespace wang {

	/**
	*	whttp_session ��http���紫��Ự(����boost.asio)
	*/
	class whttp_session
	{
	public:
		typedef std::function<void(whttp_session*)> wresponse;

	public:
		whttp_session(boost::asio::io_service& io_service);
		~whttp_session();

		// �ػ���ʼ��
		bool init(uint32 send_buf_size, uint32 recv_buf_size);

		void destroy();

		bool is_closeing() const { return m_is_closeing; }

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
		bool http_request(const std::string& host, const char* request_ptr, uint32 request_size
			, uint32 time_out_sec, wresponse func);

		void close();

		const char* get_recv_buf() const { return m_recv_buf; }
		uint32 get_recv_size() const { return m_recv_size; }

	public:
		boost::asio::ip::tcp::socket& get_socket() { return m_socket; }

		// ��ַ�����ص�
		void handle_resolve(const boost::system::error_code& err
			, boost::asio::ip::tcp::resolver::iterator endpoint_iterator);

		// ���ӻص�
		void handle_connect(const boost::system::error_code& err);

		// ��������ص�
		void handle_write_request(const boost::system::error_code& err);

		// ���ݶ�ȡ�ص�
		void handle_read_content(const boost::system::error_code& err, std::size_t bytes_transferred);

		// ʱ�ӻص�
		void check_deadline();

	private:

		// �ر�ָ������
		void _close();

		// �����ͷ���Դ
		void _release();

		// ������
		void _handle_result(uint32 error_code, std::string msg);

	private:
		whttp_session(const whttp_session& rht);
		whttp_session& operator=(const whttp_session& rht);

	private:
		bool							m_is_requesting;	// �Ƿ����첽����
		bool							m_is_timing;		// �Ƿ����ڵ���ʱ
		bool							m_is_closeing;		// �Ƿ��Ѿ����ù�socket close
		boost::asio::ip::tcp::resolver	m_resolver;			// ��ַ����
		boost::asio::deadline_timer		m_timer;			// ʱ��
		boost::asio::ip::tcp::socket	m_socket;			// session��socket��Դ

															//���ͻ���
		char*							m_send_buf;
		uint32							m_send_buf_size;
		uint32							m_send_size;

		//���ջ���
		char*							m_recv_buf;
		uint32							m_recv_buf_size;
		uint32							m_recv_size;

		wresponse						m_response;			// �ص�
	};

} // namespace wang

#endif // _NNETWORK_whttp_session_H_
