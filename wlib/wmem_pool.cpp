#include "wmem_pool.h"
#include "wlog.h"
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <sstream>

namespace wang
{

	namespace wmem_pool_util
	{
#pragma pack(push,1)

		struct wmem_block;

		// �ڴ��ͷ����
		struct wmem_node
		{
			//�����ڵ�block
			wmem_block* block_ptr;
			int next_offset; //������׵�ַ�ı���
			unsigned int nSize;
			char	buf[1];	// ʵ���ڴ�ռ�
		};
#pragma pack(pop)

#pragma pack(push,1)
		struct wmem_block
		{
		public:
			static wmem_block* create_me(unsigned short pool_index
				, unsigned int node_count
				, unsigned int node_size);
			static void release_me(wmem_block* p);

			wmem_node* take_node(unsigned int node_total);
			void return_node(wmem_node* p);
			bool empty() const { return NULL == node_free_head; }
			bool not_used() const { return 0 == node_used; }

			//block�������
			wmem_block*		next;
			wmem_block*		pre;

			//block����node��ɵ�����
			wmem_node*		node_free_head;

			unsigned short pool_index; //����
			unsigned short node_used; //���õ�node����
			unsigned int block_frequency; //ʹ�ô���

			char buf[1];//block��1M�ڴ�ռ�
		};

#pragma pack(pop)
		wmem_block* wmem_block::create_me(unsigned short pool_index
			, unsigned int node_count
			, unsigned int node_size)
		{
			const unsigned int block_size = node_count * node_size;
			wmem_block* p = static_cast<wmem_block*>(::malloc(sizeof(wmem_block) - 1 + block_size));
			if (!p)
			{
				return NULL;
			}
			p->next = NULL;
			p->pre = NULL;
			p->node_free_head = (wmem_node*)(p->buf);

			p->pool_index = pool_index;
			p->node_used = 0;
			p->block_frequency = 0;

			//��ʼ������
			{
				wmem_node* node_cur = NULL;
				int offset = 0;
				for (unsigned int i = 0; i < node_count; ++i)
				{
					node_cur = (wmem_node*)(p->buf + offset);
					node_cur->block_ptr = p;

					offset += node_size;
					node_cur->next_offset = offset;

					node_cur->nSize = node_size;
				}
				node_cur->next_offset = -1;
			}

			return p;
		}
		void wmem_block::release_me(wmem_block* p)
		{
			::free(p);
		}
		wmem_node* wmem_block::take_node(unsigned int node_total)
		{
			assert(node_free_head);

			wmem_node* p = node_free_head;
			if (-1 != p->next_offset)
			{
				//����һ��
				node_free_head = (wmem_node*)(buf + p->next_offset);
				p->next_offset = -1;
			}
			else
			{
				//û����һ����
				node_free_head = NULL;
			}

			//ʹ��Ƶ�ʼ���
			++block_frequency;
			++node_used;

			return p;
		}
		void wmem_block::return_node(wmem_node* p)
		{
			//���뵽��ǰ��
			if (node_free_head)
			{
				p->next_offset = static_cast<int>(((char*)node_free_head - buf));
			}
			node_free_head = p;

			--node_used;
		}


		class wblock_pool
		{
		public:
			wblock_pool();
			void init(unsigned int index, unsigned int node_size, unsigned int node_total);
			wmem_node* take_node(bool& flag);
			void return_node(wmem_node* ptr);
			void release_block(wmem_block* p);
			wmem_block* get_release_block();
			void move_to_frount(wmem_block* p);
			unsigned int gc();

			bool empty() const;
			unsigned short node_total() const { return m_node_total; }
			unsigned int node_size() const { return m_node_size; }

		private:
			//�Ƴ��б�
			void _remove_block(wmem_block* p);

		private:
			wmem_block*		m_head;
			wmem_block*		m_tail;
			unsigned short m_pool_index; // ����		
			unsigned short m_node_total; //ÿ��block��node����
			unsigned int m_node_size; //ÿ��node�Ĵ�С
		};

		wblock_pool::wblock_pool() :m_head(NULL), m_tail(NULL)
			, m_pool_index(0), m_node_total(0), m_node_size(0)
		{

		}
		void wblock_pool::init(unsigned int index, unsigned int node_size, unsigned int node_total)
		{
			m_pool_index = index;
			m_node_size = node_size;
			m_node_total = node_total;
		}
		wmem_node* wblock_pool::take_node(bool& flag)
		{
			if (m_head)
			{
				wmem_node* pNode = m_head->take_node(m_node_total);
				if (m_head->empty())
				{
					//û�п��п�, ������ժ��
					wmem_block* block_ptr = m_head;
					m_head = block_ptr->next;
					if (m_head)
					{
						m_head->pre = NULL;
						block_ptr->next = NULL;
					}
					else
					{
						//�����
						m_tail = NULL;
					}
				}
				return pNode;
			}

			wmem_block* block_ptr = wmem_block::create_me(m_pool_index, m_node_total, m_node_size);
			if (!block_ptr)
			{
				return NULL;
			}
			m_head = block_ptr;
			m_tail = block_ptr;
			flag = true;
			return block_ptr->take_node(m_node_total);
		}
		void wblock_pool::return_node(wmem_node* ptr)
		{
			wmem_block* block_ptr = ptr->block_ptr;
			bool flag_empty = block_ptr->empty();
			block_ptr->return_node(ptr);

			if (flag_empty && !block_ptr->empty())
			{
				//������ж��ж�β
				if (m_tail)
				{
					block_ptr->pre = m_tail;
					m_tail->next = block_ptr;
					m_tail = block_ptr;
				}
				else
				{
					m_head = block_ptr;
					m_tail = block_ptr;
				}
			}
		}

		void wblock_pool::release_block(wmem_block* p)
		{
			_remove_block(p);
			wmem_block::release_me(p);
		}
		wmem_block* wblock_pool::get_release_block()
		{
			wmem_block* block_ptr = NULL;
			for (wmem_block* p = m_head; p; p = p->next)
			{
				if (p->not_used())
				{
					if (0 == p->block_frequency)
					{
						return p;
					}
					if (!block_ptr || p->block_frequency < block_ptr->block_frequency)
					{
						block_ptr = p;
					}
				}
			}
			return block_ptr;
		}
		void wblock_pool::move_to_frount(wmem_block* p)
		{
			assert(m_head);
			//�˿��ڴ�ȫ��node��û��ʹ��, ������ǰ�ƶ�, ������ظ�������
			if (p == m_head)
			{
				//����ͷ���, �����ƶ�
				return;
			}
			if (m_head->not_used())
			{
				//ͷ���Ҳ��ȫ��node��û��ʹ��, ����ͷ���֮ǰ
				_remove_block(p);

				m_head->pre = p;
				p->next = m_head;
				p->pre = NULL;
				m_head = p;
				return;
			}
			//���� head ֮��
			if (m_head->next == p)
			{
				//����ͷ���֮�� ���ö�
				return;
			}
			//����head֮��, ��ʱ����3���ڵ�
			_remove_block(p);
			//��ʱ����2���ڵ�,����head����, ����head��tail������ı�
			m_head->next->pre = p;
			p->next = m_head->next;
			m_head->next = p;
			p->pre = m_head;
		}
		unsigned int wblock_pool::gc()
		{
			unsigned int count = 0;
			wmem_block* block_ptr = m_tail;
			while (block_ptr)
			{
				if (block_ptr->not_used())
				{
					//�ͷ�
					wmem_block* p = block_ptr;
					block_ptr = block_ptr->pre;
					release_block(p);
					++count;
				}
				else
				{
					//������ʹ�õ�
					block_ptr = block_ptr->pre;
				}
			}
			return count;
		}
		void wblock_pool::_remove_block(wmem_block* p)
		{
			wmem_block* block_ptr = p->pre;
			if (block_ptr)
			{
				block_ptr->next = p->next;
			}

			if (p->next)
			{
				p->next->pre = block_ptr;
			}

			if (m_tail == p)
			{
				m_tail = block_ptr;
			}

			if (m_head == p)
			{
				m_head = p->next;
			}
		}
		bool wblock_pool::empty() const
		{
			return m_head == NULL;
		}
	}


	using wmem_pool_util::wmem_node;
	using wmem_pool_util::wmem_block;

	wmem_pool::wmem_pool(bool safe)
		: m_pools(NULL)
		, m_max_block(0), m_block_size(0), m_start_size(0), m_pool_count(0)
		, m_current_size(0), m_used_size(0), m_block_count(0), m_block_free(0)
		, m_safe(safe)
	{

	}

	bool wmem_pool::init(unsigned int max_block/* = 10*/
		, unsigned int block_size/* = 1024 * 1024 */
		, unsigned int start_size/* = 32 */
	)
	{
		if (start_size < 32)
		{
			LOG_ERROR << "error, start_size < 32";
			return false;
		}
		if (start_size % 32 != 0)
		{
			LOG_ERROR << "error, start_size mod 32 != 0";
			return false;
		}
		if (block_size % start_size != 0)
		{
			LOG_ERROR << "error, block_size mod start_size != 0";
			return false;
		}
		m_max_block = max_block;
		m_block_size = block_size;
		m_start_size = start_size;

		unsigned int node_size = start_size;
		while (node_size < block_size)
		{
			++m_pool_count;
			node_size *= 2;
		}

		m_pools = new wmem_pool_util::wblock_pool[m_pool_count];
		if (!m_pools)
		{
			LOG_ERROR << "error, new wblock_pool failed\n";
			return false;
		}

		node_size = start_size;
		for (unsigned int i = 0; i < m_pool_count; ++i)
		{
			m_pools[i].init(i, node_size, block_size / node_size);
			node_size *= 2;
		}
		return true;
	}

	void wmem_pool::destroy()
	{
		if (m_pools)
		{
			delete[] m_pools;
			m_pools = NULL;
		}
	}

	//-----------------------------------------------------------------------------
	// ����
	//-----------------------------------------------------------------------------
	void* wmem_pool::alloc(unsigned int bytes)
	{
		unsigned int realSize = bytes + sizeof(wmem_node) - 1;
		int index = _size2index(realSize);

		if (-1 != index)
		{
			bool new_flag = false;
			wmem_node* pNode = NULL;
			if (m_safe)
			{
				m_lock.lock();
			}
			pNode = m_pools[index].take_node(new_flag);
			if (!pNode)
			{
				if (m_safe)
				{
					m_lock.unlock();
				}
				return NULL;
			}
			if (new_flag)
			{
				m_current_size += m_block_size;
				++m_block_count;
			}
			else if (1 == pNode->block_ptr->node_used)
			{
				--m_block_free;
			}
			m_current_size -= realSize;
			m_used_size += realSize;
			if (m_safe)
			{
				m_lock.unlock();
			}
			return pNode->buf;
		}

		// ��ʵ���ڴ��з���
		wmem_node* pNodePtr = reinterpret_cast<wmem_node*>(::malloc(realSize));
		if (!pNodePtr)
		{
			return NULL;
		}

		pNodePtr->block_ptr = NULL;
		pNodePtr->next_offset = -1;
		pNodePtr->nSize = realSize;
		m_used_size += realSize;
		return pNodePtr->buf;
	}

	//-----------------------------------------------------------------------------
	// �ͷ�
	//-----------------------------------------------------------------------------

	void wmem_pool::free(void* const pMem)
	{
		wmem_node* pNodePtr = reinterpret_cast<wmem_node*>(((char*)pMem) - sizeof(wmem_node) + 1);
		m_used_size -= pNodePtr->nSize;
		if (NULL != pNodePtr->block_ptr)
		{
			if (m_safe)
			{
				m_lock.lock();
			}
			if (m_block_count > m_max_block && m_block_free > 0)
			{
				// �����ռ�
				_gc_one();
			}

			wmem_block* block_ptr = pNodePtr->block_ptr;
			m_pools[block_ptr->pool_index].return_node(pNodePtr);
			if (block_ptr->not_used())
			{
				++m_block_free;
				m_pools[block_ptr->pool_index].move_to_frount(block_ptr);
			}

			m_current_size += pNodePtr->nSize;
			if (m_safe)
			{
				m_lock.unlock();
			}
			return;
		}

		::free(pNodePtr);
	}

	//-----------------------------------------------------------------------------
	// �����ռ�
	//-----------------------------------------------------------------------------

	void wmem_pool::gc()
	{
		if (m_block_free <= 0)
		{
			return;
		}
		if (m_safe)
		{
			m_lock.lock();
		}
		{
			unsigned int count = 0;
			for (unsigned int i = 0; i < m_pool_count; ++i)
			{
				if (m_pools[i].empty())
				{
					continue;
				}

				count = m_pools[i].gc();
				if (count == 0)
				{
					continue;
				}

				if (count > 0)
				{
					m_current_size -= (m_block_size * count);
					m_block_count -= count;
					m_block_free -= count;
					if (0 == m_block_free)
					{
						break;
					}
				}
			}
		}
		if (m_safe)
		{
			m_lock.unlock();
		}
	}

	std::string wmem_pool::get_info() const
	{
		std::stringstream ss;
		ss << "pool used size = " << get_used_size() << '\n';
		ss << "pool free size = " << get_size() << '\n';
		ss << "pool block_count = " << get_block_count() << '\n';
		ss << "pool block_free = " << get_block_free();
		return ss.str();
	}

	void wmem_pool::show_info() const
	{
		LOG_INFO << get_info();
	}

	int wmem_pool::_size2index(unsigned int& realSize)
	{
		for (unsigned int i = 0; i < m_pool_count; ++i)
		{
			if (realSize <= m_pools[i].node_size())
			{
				realSize = m_pools[i].node_size();
				return i;
			}
		}
		return -1;
	}

	//-----------------------------------------------------------------------------
	// �����ռ�
	//-----------------------------------------------------------------------------

	void wmem_pool::_gc_one()
	{
		// �����Ŀ�ʼ����
		wmem_block* p = NULL;
		for (unsigned int i = 0; i < m_pool_count; ++i)
		{
			if (m_pools[i].empty())
			{
				continue;
			}

			wmem_block* q = m_pools[i].get_release_block();
			if (q)
			{
				if (0 == q->block_frequency)
				{
					p = q;
					break;
				}

				if (!p || q->block_frequency < p->block_frequency)
				{
					p = q;
				}
			}
		}

		if (p)
		{
			m_pools[p->pool_index].release_block(p);
			m_current_size -= m_block_size;
			--m_block_count;
			--m_block_free;
		}
	}
}