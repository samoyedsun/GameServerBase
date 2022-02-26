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
		// �����ռ�
		void _gc_one();

	private:
		wmem_pool_util::wblock_pool*	m_pools;
		unsigned int	m_max_block;				// �ⲿ�趨�����block����
		unsigned int	m_block_size;				// �ⲿ�趨��block��С
		unsigned int	m_start_size;				// �ⲿ�趨����ʼ��С
		unsigned int	m_pool_count;				// ����õ��ĳ�������
		unsigned int	m_current_size;				// �ڴ���п����ڴ�����
		unsigned int	m_used_size;				// �ڴ�����ѱ�ʹ���ڴ�����
		unsigned int	m_block_count;				// �ѷ���block����, ÿ��1Mb
		unsigned int	m_block_free;				// ���е�block����
		std::mutex		m_lock;
		bool			m_safe;
	};
}
#endif
