/********************************************************************
created:	2015/01/20

author:		wanghaibo

purpose:	循环缓冲区


*********************************************************************/
#ifndef W_CYCLE_STREAM_H
#define W_CYCLE_STREAM_H

namespace wang {

	class wcycle_buffer
	{
	public:
		wcycle_buffer();
		wcycle_buffer(char * buf, unsigned int size);
		~wcycle_buffer();

	public:
		//初始化
		bool init(char * buf, unsigned int size);

		void reset();

		bool peek(void* buf, unsigned int len);

		//跳过指定长度缓冲
		bool skip(unsigned int len);

		bool read(void* buf, unsigned int len);

		bool write(const void* buf, unsigned int len);

		bool empty() const { return m_cur_len == 0; }

		unsigned int avail() const { return m_buf_size - m_cur_len; }

		//获得buf长度
		unsigned int length() const { return m_cur_len; }

	private:
		wcycle_buffer(const wcycle_buffer&);
		wcycle_buffer& operator=(const wcycle_buffer&);

	private:
		char*			m_buf;			//缓冲区
		unsigned int	m_buf_size;		//缓冲区总长度
		unsigned int	m_head;		//头标记
		unsigned int	m_tail;		//尾标记
		unsigned int	m_cur_len;		//缓冲区数据长度
	};

}
#endif
