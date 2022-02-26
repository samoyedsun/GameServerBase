#pragma once
/********************************************************************
created:	copyright fast time Inc

author:		WangHaibo

purpose:	msg create realse execute
*********************************************************************/


#ifndef W_NET_MSG_STATISTIC_H
#define W_NET_MSG_STATISTIC_H

#include "wsingleton.h"
#include <string>
#include <atomic>

namespace wang
{
	class wasync_log;

	class wnet_msg_statistic
	{
	private:
		struct wstatistic
		{
			const char* msg_name;
			std::atomic_ullong msgCount;
			std::atomic_ullong msgSize;
			wstatistic() : msg_name(NULL), msgCount(0), msgSize(0) { }
		};
		//-- constructor/destructor
	public:
		//[min_msg_id, max_msg_id)
		wnet_msg_statistic();
		~wnet_msg_statistic();

		//-- member function
	public:
		bool init(unsigned int max_msg_id, unsigned int show_interval);
		void destroy();

		void update(unsigned int elapse);

		bool register_msg(unsigned short msg_id, const char* msg_name);

		void count_msg(unsigned short msg_id, unsigned int msg_size);

		void write_out();

		const char* get_msg_name(unsigned short msg_id) const;

	private:
		wnet_msg_statistic(const wnet_msg_statistic& rht);
		wnet_msg_statistic& operator=(const wnet_msg_statistic& rht);

		bool _invalid_msg(unsigned short msg_id) const;
		bool _init_log();

	private:
		wasync_log*		m_log;
		wstatistic*		m_msgs;
		unsigned int	m_count;
		unsigned int	m_interval;
		unsigned int	m_cur;
	};

#define s_net_msg_statistic wsingleton<wnet_msg_statistic>::get_instance()

}

#endif														