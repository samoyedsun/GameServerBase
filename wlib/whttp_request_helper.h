/**
* ∑¢ÀÕHTTPÕ¯¬Á«Î«Û¿‡
*/
#ifndef W_HTTP_REQUEST_HELPER_H
#define W_HTTP_REQUEST_HELPER_H

#include "wnet_type.h"
#include <string>

namespace wang {

	enum EHttpResponseCode
	{
		EHRCode_Succ = 200,
		EHRCode_Timeout = 801,
		EHRCode_Resolve = 802,
		EHRCode_Connect = 803,
		EHRCode_Write = 804,
		EHRCode_Err = 805,
		EHRCode_JsonPassErr = 806,
	};

	void http_address_parse(const std::string& address, std::string& server_address, std::string& method_path);

	namespace whttp_request_helper
	{
		class whttp_para
		{
		public:
			whttp_para();
			~whttp_para();

		public:
			bool init(uint32 buf_size);
			void destroy();

		public:
			const char* get_buf() const { return m_buf; }

			uint32 get_size() const { return m_pos; }

			bool has_error() const { return m_error; }

			bool no_error() const { return !has_error(); }

			void reset();

		private:
			void write(const void* buf, uint32 len);
			void set_error();

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
			void write_value(const char* p, uint32 len);

		private:
			char*			m_buf;
			char			m_temp_buf[24];
			uint32			m_buf_size;
			uint32			m_pos;
			bool			m_error;
		};


		class whttp_request
		{
		public:
			whttp_request();
			~whttp_request();

		public:
			bool init(uint32 buf_size);
			void destroy();

		public:
			const char* get_buf() const { return m_buf; }

			uint32 get_size() const { return m_pos; }

			bool has_error() const { return m_error; }

			bool no_error() const { return !has_error(); }

			void reset();

			whttp_para& get_para() { return m_para; }

			void make_get(const std::string& host, const std::string& path, const std::string& filename);
			void make_post(const std::string& host, const std::string& path, const std::string& filename);

		private:
			void write(const void* buf, uint32 len);

			whttp_request& operator<<(const char& value);
			whttp_request& operator<<(const unsigned int& value);
			whttp_request& operator<<(const char* c_str);
			whttp_request& operator<<(const std::string& value);

			whttp_request& operator<<(const whttp_para& value);

			void set_error();

		private:
			whttp_request(const whttp_request&);
			whttp_request& operator=(const whttp_request&);

		private:
			char*			m_buf;
			char			m_temp_buf[24];
			uint32			m_buf_size;
			uint32			m_pos;
			bool			m_error;
			whttp_para		m_para;
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
