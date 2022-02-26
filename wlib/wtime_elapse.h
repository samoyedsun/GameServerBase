#ifndef W_TIME_ELAPSE_H
#define W_TIME_ELAPSE_H

#include "wtime_api.h"

namespace wang {

	class wtime_elapse
	{
	public:
		wtime_elapse() : m_cur_time(0)
		{
			m_cur_time = wtime_api::get_time_ms();
		}

		unsigned int get_elapse()
		{
			long long last_time = m_cur_time;
			m_cur_time = wtime_api::get_time_ms();
			if (m_cur_time >= last_time)
			{
				return static_cast<unsigned int>(m_cur_time - last_time);
			}
			else
			{
				return 1;
			}
		}

	private:
		long long m_cur_time;
	};

} // namespace wang

#endif // W_TIME_ELAPSE_H
