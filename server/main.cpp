#include <iostream>
#include <map>
#include <boost/version.hpp>
#include "wthread_pool.h"

using namespace std;

namespace wang
{
	static std::map<unsigned int, int> mmap;
	void task()
	{
		hash<std::thread::id> h;
		unsigned int tid = h(std::this_thread::get_id()) % 128;
		mmap[tid] = mmap[tid] + 1;
		std::cout << "I'm a task! tid:" << tid << endl;
	}

	void space()
	{
		std::cout << "Using Boost "
			<< BOOST_VERSION / 100000 << "."
			<< BOOST_VERSION / 100 % 1000 << "."
			<< BOOST_VERSION % 100
			<< std::endl;

		wthread_pool pool;
		pool.start(8);
		for (int i = 0; i < 100; ++i)
		{
			pool.post_task(task);
		}
		getchar();
		pool.shutdown();
		getchar();
		for (auto iter = mmap.begin(); iter != mmap.end(); ++iter)
		{
			cout << "tid:" << iter->first << "count:" << iter->second << endl;
		}
	}
}

int main(void)
{
	wang::space();

	system("pause");
	return 0;
}

