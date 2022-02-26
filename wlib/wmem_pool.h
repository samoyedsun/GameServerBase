#ifndef W_MEM_POOL_H
#define W_MEM_POOL_H

#include <mutex>
#include <string>

namespace wang
{
	namespace wmem_pool_util
	{
		class wblock_pool;
	}

	class wmem_pool
	{
	public:
		explicit wmem_pool(bool safe = true);

		bool init(unsigned int max_block = 10
			, unsigned int block_size = 1024 * 1024
			, unsigned int start_size = 32);
		void destroy();

		void* alloc(unsigned int bytes);
		void free(void* const ptr);

		void gc();

		unsigned int get_max_block()	const { return m_max_block; }
		unsigned int get_size()	const { return m_current_size; }
		unsigned int get_used_size() const { return m_used_size; }
		unsigned int get_block_count() const { return m_block_count; }
		unsigned int get_block_free() const { return m_block_free; }
		void set_max_block(unsigned int max_block) { m_max_block = max_block; }
		std::string get_info() const;
		void show_info() const;

	private:
		int _size2index(unsigned int& realSize);
		// 垃圾收集
		void _gc_one();

	private:
		wmem_pool_util::wblock_pool*	m_pools;
		unsigned int	m_max_block;				// 外部设定的最大block数量
		unsigned int	m_block_size;				// 外部设定的block大小
		unsigned int	m_start_size;				// 外部设定的起始大小
		unsigned int	m_pool_count;				// 计算得到的池子数量
		unsigned int	m_current_size;				// 内存池中空闲内存总数
		unsigned int	m_used_size;				// 内存池中已被使用内存总数
		unsigned int	m_block_count;				// 已分配block数量, 每个1Mb
		unsigned int	m_block_free;				// 空闲的block数量
		std::mutex		m_lock;
		bool			m_safe;
	};
}
#endif
