/********************************************************************
created:	2015/04/10

author:		wanghaibo
desc:		cycle queue with fix size
*********************************************************************/

#ifndef W_SAFE_CYCLE_QUEUE_H
#define W_SAFE_CYCLE_QUEUE_H

#include <mutex>
#include "wcycle_queue.h"

namespace wang
{
	template<typename T>
	class wsafe_cycle_queue : public wcycle_queue<T>
	{
		//-- member function
	public:
		bool push(const T& data);
		bool pop(T& data);

	private:
		std::mutex m_mutex;
	};

	template<typename T>
	bool wsafe_cycle_queue<T>::push(const T& data)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		return wcycle_queue<T>::push(data);
	}

	template<typename T>
	bool wsafe_cycle_queue<T>::pop(T& data)
	{
		std::lock_guard<std::mutex> guard(m_mutex);
		return wcycle_queue<T>::pop(data);
	}
}
#endif