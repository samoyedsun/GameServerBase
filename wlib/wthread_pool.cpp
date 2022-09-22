#include "wthread_pool.h"

namespace wang
{
	wthread_pool::wthread_pool()
		: m_stop(false)
	{
	}

	wthread_pool::~wthread_pool()
	{
	}

	void wthread_pool::start(size_t size)
	{
		for (int i = 0; i < size; ++i)
		{
			m_threads.emplace_back(&wthread_pool::schedual, this);
		}
	}

	void wthread_pool::post_task(wTask task)
	{
		std::lock_guard<std::mutex> lock(m_lock);
		if (m_stop)
		{
			return;
		}
		m_tasks.push(task);
		m_condition.notify_one();
	}

	void wthread_pool::shutdown()
	{
		m_stop = true;
		m_condition.notify_all();
		for (std::thread& thread : m_threads)
		{
			if (thread.joinable())
			{
				thread.join();
			}
		}
	}

	void wthread_pool::schedual()
	{
		while (!m_stop)
		{
			std::unique_lock<std::mutex> lock(m_lock);
			m_condition.wait(lock);
			if (!m_tasks.empty())
			{
				wTask task = m_tasks.back();
				task();
				m_tasks.pop();
			}
		}
	}

	wthread_pool::wthread_pool(const wthread_pool& rht)
	{
		*this = rht;
	}

	wthread_pool& wthread_pool::operator=(const wthread_pool& rht)
	{
		//m_threads = rht.m_threads;
		//m_tasks = rht.m_tasks;
		return *this;
	}
}