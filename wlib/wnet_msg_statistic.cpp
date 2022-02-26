#include "wnet_msg_statistic.h"
#include "wtime_const.h"
#include "wtime_api.h"
#include "wlog.h"
#include "wasync_log.h"
#include <sstream>

namespace wang
{

	wnet_msg_statistic::wnet_msg_statistic()
		: m_log(nullptr)
		, m_msgs(nullptr)
		, m_count(0)
		, m_interval(0)
		, m_cur(0)
	{

	}

	wnet_msg_statistic::~wnet_msg_statistic()
	{

	}

	bool wnet_msg_statistic::init(unsigned int max_msg_id, unsigned int show_interval)
	{
		if (m_msgs)
		{
			return true;
		}

		m_msgs = new wstatistic[max_msg_id];
		if (!m_msgs)
		{
			LOG_ERROR << "failed";
			return false;
		}

		m_count = max_msg_id;

		m_interval = show_interval;

		if (m_interval > 0 && !_init_log())
		{
			return false;
		}

		return true;
	}

	void wnet_msg_statistic::destroy()
	{
		write_out();
		if (m_log)
		{
			m_log->destroy();
			delete m_log;
			m_log = nullptr;
		}
		if (m_msgs)
		{
			delete[]  m_msgs;
			m_msgs = nullptr;
		}
	}

	void wnet_msg_statistic::update(unsigned int elapse)
	{
		if (m_interval)
		{
			m_cur += elapse;
			if (m_cur >= m_interval)
			{
				m_cur -= m_interval;
				write_out();
			}
		}
	}

	bool wnet_msg_statistic::register_msg(unsigned short msg_id, const char* msg_name)
	{
		if (!m_msgs)
		{
			LOG_ERROR << "not init";
			return false;
		}
		if (_invalid_msg(msg_id))
		{
			LOG_ERROR << "invalid msg id " << msg_id << ", msg_name=" << msg_name;
			return false;
		}

		if (m_msgs[msg_id].msg_name != NULL)
		{
			LOG_ERROR << "msg " << msg_name << " has been register";
			return false;
		}

		m_msgs[msg_id].msg_name = msg_name;
		return true;
	}

	void wnet_msg_statistic::count_msg(unsigned short msg_id, unsigned int msg_size)
	{
		if (!m_msgs)
		{
			return;
		}
		if (!_invalid_msg(msg_id))
		{
			++m_msgs[msg_id].msgCount;
			m_msgs[msg_id].msgSize += msg_size;
		}
	}

	void wnet_msg_statistic::write_out()
	{
		if (!m_msgs)
		{
			return;
		}

		std::stringstream ss;
		{
			//time
			char buf[ASCII_DATETIME_LEN];
			wtime_api::datetime_format(buf, '-', ' ', ':');
			ss << buf << '\n';
		}

		ss << "msg id, msg count, msg size\n";
		bool flag = false;
		unsigned long long msgCount = 0;
		unsigned long long MsgSize = 0;
		for (unsigned int i = 0; i < m_count; ++i)
		{
			wstatistic& data = m_msgs[i];
			if (data.msgCount == 0)
			{
				continue;
			}
			flag = true;
			msgCount = data.msgCount.exchange(0);
			MsgSize = data.msgSize.exchange(0);
			if (data.msg_name)
			{
				ss << data.msg_name;
			}
			else
			{
				ss << i;
			}
			ss << ',' << msgCount << ',' << MsgSize << '\n';

		}
		if (flag)
		{
			ss << '\n';
			if (!m_log && !_init_log())
			{
				return;
			}
			std::string str(ss.str());
			m_log->append_fix(ELogLevel_None, str.data(), static_cast<unsigned int>(str.length()));
		}
	}

	const char* wnet_msg_statistic::get_msg_name(unsigned short msg_id) const
	{
		if (m_msgs && !_invalid_msg(msg_id))
		{
			return m_msgs[msg_id].msg_name;
		}
		return NULL;
	}

	bool wnet_msg_statistic::_invalid_msg(unsigned short msg_id) const
	{
		return (msg_id < 0 || msg_id >= m_count);
	}

	bool wnet_msg_statistic::_init_log()
	{
		m_log = new wasync_log();
		if (!m_log)
		{
			return false;
		}
		std::string filename(wlog::get_fullname());
		{
			std::string::size_type n = filename.find_last_of(".\\/");
			if (n != std::string::npos && filename[n] == '.')
			{
				filename.erase(filename.begin() + n, filename.end());
			}
		}
		filename.append(".csv");

		return m_log->init(filename, false, false);
	}
}