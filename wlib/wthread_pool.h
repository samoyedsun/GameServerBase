#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H

#include <functional>
#include <thread>
#include <condition_variable>
#include <vector>
#include <queue>

namespace wang
{
	class wthread_pool
	{
	public:
		typedef std::function<void()> wTask;

	public:
		wthread_pool();

		~wthread_pool();

		//�����̳߳�
		void start(size_t size);

		// �ύһ������
		void post_task(wTask task);

		// ִ���������ر�
		void shutdown();

	private:
		// �������
		void schedual();

	private:
		wthread_pool(const wthread_pool& rht);
		wthread_pool& operator=(const wthread_pool& rht);

	private:
		// �̳߳�
		std::vector<std::thread> m_threads;

		// �������
		std::queue<wTask> m_tasks;
		// ͬ��
		std::mutex m_lock;
		std::condition_variable m_condition;

		// �Ƿ�ر�
		bool m_stop;
	};
}

#endif

