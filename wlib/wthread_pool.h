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

		//启动线程池
		void start(size_t size);

		// 提交一个任务
		void post_task(wTask task);

		// 执行完任务后关闭
		void shutdown();

	private:
		// 任务调度
		void schedual();

	private:
		wthread_pool(const wthread_pool& rht);
		wthread_pool& operator=(const wthread_pool& rht);

	private:
		// 线程池
		std::vector<std::thread> m_threads;

		// 任务队列
		std::queue<wTask> m_tasks;
		// 同步
		std::mutex m_lock;
		std::condition_variable m_condition;

		// 是否关闭
		bool m_stop;
	};
}

#endif

