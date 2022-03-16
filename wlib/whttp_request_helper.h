/**
* ∑¢ÀÕHTTPÕ¯¬Á«Î«Û¿‡
*/
#ifndef W_HTTP_REQUEST_HELPER_H
#define W_HTTP_REQUEST_HELPER_H

#include "wnet_type.h"
#include "wbuffer.h"
#include <string>

namespace wang {

	void http_address_parse(const std::string& url, std::string& host, std::string& method);

	void http_host_parse(const std::string& host, std::string& ip, std::string& port);

	namespace whttp_request_helper
	{
		class whttp_para : public wbuffer
		{
		public:
			whttp_para();
			~whttp_para();

		public:
			bool init(uint32 buf_size);
			void destroy();

			void write_encode(const void* p, uint32 len);

		private:
			whttp_para(const whttp_para& rht);
			whttp_para& operator=(const whttp_para& rht);

		public:
			whttp_para& operator<<(char value);

			whttp_para& operator<<(bool value);

			whttp_para& operator<<(signed char value);

			whttp_para& operator<<(unsigned char value);

			whttp_para& operator<<(signed short value);

			whttp_para& operator<<(unsigned short value);

			whttp_para& operator<<(signed int value);

			whttp_para& operator<<(unsigned int value);

			whttp_para& operator<<(signed long value);

			whttp_para& operator<<(unsigned long value);

			whttp_para& operator<<(signed long long value);

			whttp_para& operator<<(unsigned long long value);

			//must be zero end string, not write the end 0
			whttp_para& operator<<(const char* c_str);

			whttp_para& operator<<(const std::string& value);

		private:
			char	m_my_buf[1024];
			char*	m_buf;
		};

		class whttp_request : public wbuffer
		{
		public:
			whttp_request();
			~whttp_request();

		public:
			bool init(uint32 buf_size);
			void destroy();

			void make_get(const std::string& host, const std::string& method, const wbuffer& buf);
			void make_post(const std::string& host, const std::string& method, const wbuffer& buf);

		private:
			whttp_request& operator<<(const char& value);
			whttp_request& operator<<(const unsigned int& value);
			whttp_request& operator<<(const char* c_str);
			whttp_request& operator<<(const std::string& value);
			whttp_request& operator<<(const wbuffer& value);

		private:
			whttp_request(const whttp_request&);
			whttp_request& operator=(const whttp_request&);

		private:
			char	m_my_buf[1024];
			char*	m_buf;
		};

		class whttp_response
		{
		public:
			whttp_response();

		public:

			bool parse(const void* buf, uint32 len);

			int32 get_code() const { return m_code; }

			const char* get_content_ptr() const { return m_buf + m_content_index; }

			uint32 get_content_size() const { return m_content_size; }

			void set_err_msg(const std::string& err_msg) { m_err_msg = err_msg; }
			const std::string& get_err_msg() const { return m_err_msg; }

		private:
			const char*	m_buf;
			uint32		m_buf_size;
			int32		m_code;
			std::string m_err_msg;
			uint32		m_content_index;
			uint32		m_content_size;
		};


		class whttp_string_reader
		{
		public:
			whttp_string_reader(const char* buf, uint32 len);


		public:
			void get_str(std::string& str);
			void get_line(std::string& str);

		private:
			const char*	m_buf;
			uint32		m_buf_size;
			uint32		m_pos;
		};

	};
}
#endif
