/********************************************************************
created:	2015/01/20

author:		wanghaibo

purpose:	ѭ��������


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
		//��ʼ��
		bool init(char * buf, unsigned int size);

		void reset();

		bool peek(void* buf, unsigned int len);

		//����ָ�����Ȼ���
		bool skip(unsigned int len);

		bool read(void* buf, unsigned int len);

		bool write(const void* buf, unsigned int len);

		bool empty() const { return m_cur_len == 0; }

		unsigned int avail() const { return m_buf_size - m_cur_len; }

		//���buf����
		unsigned int length() const { return m_cur_len; }

	private:
		wcycle_buffer(const wcycle_buffer&);
		wcycle_buffer& operator=(const wcycle_buffer&);

	private:
		char*			m_buf;			//������
		unsigned int	m_buf_size;		//�������ܳ���
		unsigned int	m_head;		//ͷ���
		unsigned int	m_tail;		//β���
		unsigned int	m_cur_len;		//���������ݳ���
	};

}
#endif
