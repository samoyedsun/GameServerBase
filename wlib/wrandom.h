/********************************************************************
created:	2014/11/21

author:		wanghaibo

purpose:	随机数管理类 wrandom
*********************************************************************/

#ifndef W_RANDOM_H
#define W_RANDOM_H

#include "wnet_type.h"
#include "wnoncopyable.h"
#include "wsingleton.h"

namespace wang
{
	class wrandom : public wnoncopyable
	{
	public:
		wrandom();
		explicit wrandom(uint32 seed);

		void init();
		void init(uint32 seed);

		uint32 rand();

		float randf();

		//输出结果 [minValue, maxValue]
		uint32 rand(uint32 minValue, uint32 maxValue);

		//输出结果 [0, maxValue)
		uint32 rand(uint32 maxValue);

		uint32 operator()()
		{
			return rand();
		}

		uint32 operator()(int32 maxValue)
		{
			return rand(maxValue);
		}

	private:
		static const uint32 MAX_SEED_NUM = 624;

	private:
		uint32 m_MT[MAX_SEED_NUM];
		uint32 m_index;
	};

#define s_rand wsingleton<wrandom>::get_instance()
};

#endif