#ifndef W_BUFFER_H
#define W_BUFFER_H

namespace wang {

	class wbuffer
	{
	public:
		wbuffer(char * buf, unsigned int size);

		void set_buf(char * buf, unsigned int size);

	public:
		char* get_buf() { return m_buf; }
		const char* get_buf() const { return m_buf; }

		unsigned int get_size() const { return m_pos; }

		unsigned int avail() const { return m_buf_size - m_pos; }

		bool has_error() const { return m_error; }

		bool no_error() const { return !has_error(); }

		void set_error() { m_error = true; }

		void reset();

		void write(const void* buf, unsigned int len);

		void write(unsigned int pos, const void* buf, unsigned  int len);

		void skip_no_check(unsigned int len) { m_pos += len; }
		void skip(unsigned int len);

	private:
		char*			m_buf;
		unsigned int	m_buf_size;
		unsigned int	m_pos;
		bool			m_error;
	};

}
#endif // W_BUFFER_H
