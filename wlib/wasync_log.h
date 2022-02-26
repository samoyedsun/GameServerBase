/********************************************************************
created:	2016/08/15

author:		wanghaibo

purpose:	async log
*********************************************************************/

#ifndef W_ASYNC_LOG_H
#define W_ASYNC_LOG_H

#include <thread>
#include <condition_variable>
#include <fstream>
#include <string>
#include <stdarg.h>
#include "wlog_define.h"

namespace wang
{
	struct wlog_buf;
	struct wlog_item;
	class wlog_color;

	class wasync_log
	{
	public:
		wasync_log();
		~wasync_log();

		bool init(const std::string& path, const std::string& name, const std::string& ext
			, ELogNameType name_type, bool mod_append, bool show_screen);

		bool init(const std::string& fullname, bool mod_append, bool show_screen);

		void destroy();

		//◊∑º”»’÷æ
		void append_fix(ELogLevelType level, const void* str, unsigned int len);
		void append_var(ELogLevelType level, const char* format, va_list ap);

		ELogLevelType get_level() const { return m_level; }
		void set_level(ELogLevelType level) { m_level = level; }

		const std::string& get_path() const { return m_path; }
		const std::string& get_fullname() const { return m_fullname; }

	private:
		void _work_thread();
		wlog_buf* _get_next_data_buf();
		bool _push_data_buf();
		void _handler_log_item(const wlog_item* log_item_ptr);

	private:
		wasync_log(const wasync_log&);
		wasync_log& operator=(const wasync_log&);

	private:
		typedef std::condition_variable wcond;

	private:
		std::string		m_path;
		std::string		m_name;
		std::string		m_ext;
		std::string		m_fullname;

		bool			m_show_screen;
		ELogLevelType	m_level;
		int				m_timestamp;

		std::ofstream	m_file;

		std::thread		m_thread;
		std::mutex		m_lock;
		wcond			m_condition;
		volatile bool	m_stop;

		wlog_buf*		m_cur_buf;
		wlog_buf*		m_avail_head;
		wlog_buf*		m_data_head;
		wlog_buf*		m_data_tail;

		wlog_color*		m_color_ptr;
	};
}

#endif