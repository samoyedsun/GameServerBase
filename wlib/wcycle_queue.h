/********************************************************************
created:	2015/04/10

author:		wanghaibo
desc:		cycle queue with fix size
*********************************************************************/

#ifndef W_CYCLE_QUEUE_H
#define W_CYCLE_QUEUE_H

namespace wang
{
	template<typename T>
	class wcycle_queue
	{
		//-- constructor/destructor
	public:
		wcycle_queue();
		~wcycle_queue();

		//-- member function
	public:
		//-- prop
		bool init(int uMaxNum);
		void destroy();

		bool empty() const { return (0 == m_size); }
		bool full() const { return (m_size == m_maxSize); }
		int	size() const { return m_size; }
		int	get_max_size() const { return m_maxSize; }

		bool push(const T& data);
		bool pop(T& data);
		void clear();

	private:
		wcycle_queue(const wcycle_queue&);
		wcycle_queue& operator=(const wcycle_queue&);

	private:
		T*	m_datas;
		int	m_maxSize;
		int m_size;
		int	m_head;
		int	m_tail;
	};

	template<typename T>
	wcycle_queue<T>::wcycle_queue() : m_datas(NULL), m_maxSize(0), m_size(0), m_head(0), m_tail(0)
	{

	}

	template<typename T>
	wcycle_queue<T>::~wcycle_queue()
	{
		destroy();
	}

	template<typename T>
	void wcycle_queue<T>::destroy()
	{
		if (m_datas)
		{
			delete[] m_datas;
			m_datas = NULL;
		}
		m_maxSize = 0;
		m_head = 0;
		m_tail = 0;
	}

	template<typename T>
	bool wcycle_queue<T>::init(int uMaxNum)
	{
		m_datas = new T[uMaxNum];
		if (!m_datas)
		{
			return false;
		}
		m_maxSize = uMaxNum;
		return true;
	}

	template<typename T>
	bool wcycle_queue<T>::push(const T& data)
	{
		if (!full())
		{
			m_datas[m_tail] = data;
			m_tail = (m_tail + 1) % m_maxSize;
			++m_size;
			return true;
		}
		return false;
	}

	template<typename T>
	bool wcycle_queue<T>::pop(T& data)
	{
		if (!empty())
		{
			data = m_datas[m_head];
			m_head = (m_head + 1) % m_maxSize;
			--m_size;
			return true;
		}
		return false;
	}

	template<typename T>
	void wcycle_queue<T>::clear()
	{
		m_size = 0;
		m_head = 0;
		m_tail = 0;
	}

}
#endif