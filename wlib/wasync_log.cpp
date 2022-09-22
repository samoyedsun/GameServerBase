#include "wasync_log.h"
#include "wtime_api.h"
#include "wtime_const.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <cstring>

#ifdef _MSC_VER
#pragma warning (disable:4996)
#include <windows.h>
#endif

namespace wang
{
#pragma pack(push,1)
	struct wlog_item
	{
		int timestamp;
		unsigned int size;
		signed char level;
		char		 buf[1];
	};
#pragma pack(pop)

	//多一个字节放置换行符
	static const unsigned int LOG_ITEM_SIZE = sizeof(wlog_item);

	static const unsigned int LOG_BUF_MAX_SIZE = 1024 * 1024;

	static const unsigned int LOG_MAX_LEN = LOG_BUF_MAX_SIZE - LOG_ITEM_SIZE;

	struct wlog_buf
	{
		wlog_buf*	 next;
		unsigned int size;
		char		 buf[LOG_BUF_MAX_SIZE];

		wlog_buf() : next(NULL), size(0)
		{
			buf[0] = 0;
		}
	};

	enum ELogColorType
	{
		ELCT_White = 0,
		ELCT_Red,
		ELCT_Pink,
		ELCT_Yellow,
		ELCT_Blue,
		ELCT_Green,
		ELCT_Max
	};

	class wlog_color
	{
	public:
		wlog_color();

		void set_color(ELogColorType color);

	private:
		int	m_color_fg[ELCT_Max];
	};

	wlog_color::wlog_color()
	{
#if defined(_MSC_VER)
		m_color_fg[ELCT_White] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		m_color_fg[ELCT_Red] = FOREGROUND_RED | FOREGROUND_INTENSITY;
		m_color_fg[ELCT_Pink] = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		m_color_fg[ELCT_Green] = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		m_color_fg[ELCT_Blue] = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		m_color_fg[ELCT_Yellow] = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

#elif defined(__GNUC__)

		enum ANSIFgTextAttr
		{
			FG_BLACK = 30, FG_RED, FG_GREEN, FG_YELLOW, FG_BLUE,
			FG_MAGENTA, FG_CYAN, FG_WHITE,
		};

		m_color_fg[ELCT_White] = FG_WHITE;
		m_color_fg[ELCT_Red] = FG_RED;
		m_color_fg[ELCT_Pink] = FG_CYAN;
		m_color_fg[ELCT_Green] = FG_GREEN;
		m_color_fg[ELCT_Blue] = FG_BLUE;
		m_color_fg[ELCT_Yellow] = FG_YELLOW;

#else
#pragma error "unknow platform!!!"

#endif
	}

	void wlog_color::set_color(ELogColorType color)
	{
#if defined(_MSC_VER)
		::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), m_color_fg[color]);
#elif defined(__GNUC__)
		//fprintf(stdout, "\033[0;40;%dm", m_color_fg[color]);
		//static const X_CHAR* colorString[TBLUE+1] = {"","\033[22;31m","\033[22;32m","\033[01;33m","\033[0m","\033[01;37m","\033[01;34m",};
		//fputs(colorString[color],stdout); 
#endif
	}

	struct wlog_name_color
	{
		const char* name;
		ELogColorType color;
	}
	static const g_log_name_colors[] =
	{
		{ "",		ELCT_White },
		{ "system", ELCT_Blue },
		{ "fatal",  ELCT_Red },
		{ "error",  ELCT_Pink },
		{ "warn",	ELCT_Yellow },
		{ "info",	ELCT_White },
		{ "debug",	ELCT_Green },
	};

	static void gen_log_file_name(char* p, const std::string& path, const std::string& name
		, const std::string& ext, ELogNameType name_type, int& day_stamp)
	{
		p += sprintf(p, "%s", path.c_str());
		if (name_type == ELogName_Date)
		{
			p += wtime_api::datetime_format(p, 0, -1, -1);
			*p++ = '_';
		}
		else if (name_type == ELogName_DateTime)
		{
			p += wtime_api::datetime_format(p, 0, 0, 0);
			*p++ = '_';
		}
		else if (name_type == ELogName_AutoDate)
		{
			if (day_stamp == 0)
			{
				day_stamp = wtime_api::get_today_stamp_time32(0);
			}
			p += wtime_api::time32_datetime_format(day_stamp, p, 0, -1, -1);
			*p++ = '_';
			day_stamp += ETC_Day;
		}
		sprintf(p, "%s%s", name.c_str(), ext.c_str());
	}

	wasync_log::wasync_log()
		: m_show_screen(0)
		, m_level(ELogLevel_Num)
		, m_timestamp(0)
		, m_stop(true)
		, m_cur_buf(NULL)
		, m_avail_head(NULL)
		, m_data_head(NULL)
		, m_data_tail(NULL)
		, m_color_ptr(NULL)
	{

	}

	wasync_log::~wasync_log()
	{
		destroy();
	}

	bool wasync_log::init(const std::string& path, const std::string& name, const std::string& ext
		, ELogNameType name_type, bool mod_append, bool show_screen)
	{
		m_path = path;
		if (!m_path.empty() && m_path.back() != '\\' && m_path.back() != '/')
		{
			m_path.append(1, '/');
		}
		if (!m_path.empty())
		{
			//检查目录是否存在, 不存在则创建
			boost::filesystem::path log_path(m_path);
			if (!boost::filesystem::exists(log_path))
			{
				boost::system::error_code ec;
				boost::filesystem::create_directories(log_path, ec);
			}
		}
		m_name = name;
		if (!ext.empty() && ext.front() != '.')
		{
			m_ext = '.';
			m_ext += ext;
		}
		else
		{
			m_ext = ext;
		}

		char filename[1024] = { 0 };
		gen_log_file_name(filename, m_path, m_name, m_ext, name_type, m_timestamp);
		return init(filename, mod_append, show_screen);
	}

	bool wasync_log::init(const std::string& fullname, bool mod_append, bool show_screen)
	{
		m_fullname.assign(fullname);
		if (mod_append)
		{
			m_file.open(m_fullname.c_str(), std::ios_base::app);
		}
		else
		{
			m_file.open(m_fullname.c_str());
		}
		if (!m_file.is_open())
		{
			std::cerr << "open log file " << m_fullname.c_str() << " failed" << std::endl;
			return false;
		}

		m_cur_buf = new wlog_buf();
		if (!m_cur_buf)
		{
			std::cerr << "new cur buff failed" << std::endl;
			return false;
		}

		m_avail_head = new wlog_buf();
		if (!m_avail_head)
		{
			std::cerr << "new avail buff failed" << std::endl;
			return false;
		}

		m_show_screen = show_screen;

		if (show_screen)
		{
			m_color_ptr = new wlog_color();
			if (!m_color_ptr)
			{
				std::cerr << "new wlog_color failed" << std::endl;
				return false;
			}
		}

		m_stop = false;
		std::thread td(std::bind(&wasync_log::_work_thread, this));
		m_thread.swap(td);

		return true;
	}

	void wasync_log::destroy()
	{
		{
			std::lock_guard<std::mutex> lock(m_lock);
			if (m_stop)
			{
				return;
			}
			m_stop = true;
		}
		m_condition.notify_all();

		if (m_thread.joinable())
		{
			m_thread.join();
		}

		if (m_file.is_open())
		{
			m_file.close();
		}

		if (m_show_screen)
		{
			std::cout.flush();
		}

		if (m_cur_buf)
		{
			delete m_cur_buf;
			m_cur_buf = NULL;
		}

		while (m_avail_head)
		{
			wlog_buf* p = m_avail_head;
			m_avail_head = m_avail_head->next;
			delete p;
		}

		while (m_data_head)
		{
			wlog_buf* p = m_data_head;
			m_data_head = m_data_head->next;
			delete p;
			if (!m_data_head)
			{
				m_data_tail = NULL;
			}
		}
		if (m_color_ptr)
		{
			delete m_color_ptr;
			m_color_ptr = NULL;
		}
	}

	void wasync_log::append_fix(ELogLevelType level, const void* str, unsigned int len)
	{
		if (level > m_level || level < ELogLevel_None || level >= ELogLevel_Num)
		{
			return;
		}
		if (len > LOG_MAX_LEN)
		{
			//溢出 截断
			len = LOG_MAX_LEN;
		}
		{
			std::lock_guard<std::mutex> lock(m_lock);
			if (m_stop)
			{
				return;
			}

			if (m_cur_buf->size + LOG_ITEM_SIZE + len > LOG_BUF_MAX_SIZE)
			{
				//当前buf写不下了, 申请一个新的buf
				if (!_push_data_buf())
				{
					return;
				}
			}

			//write str to buf
			{
				wlog_item log_item;
				log_item.timestamp = wtime_api::get_time32();
				log_item.level = level;
				log_item.size = len;

				memcpy(m_cur_buf->buf + m_cur_buf->size, &log_item, LOG_ITEM_SIZE - 1);
				m_cur_buf->size += (LOG_ITEM_SIZE - 1);

				memcpy(m_cur_buf->buf + m_cur_buf->size, str, len);
				m_cur_buf->size += len;
			}
		}
		{
			m_condition.notify_one();
		}
	}

	void wasync_log::append_var(ELogLevelType level, const char* format, va_list ap)
	{
		if (level > m_level || level < ELogLevel_None || level >= ELogLevel_Num)
		{
			return;
		}
		{
			std::lock_guard<std::mutex> lock(m_lock);
			if (m_stop)
			{
				return;
			}

			static const unsigned int max_len = 1024;

			if (m_cur_buf->size + LOG_ITEM_SIZE + max_len > LOG_BUF_MAX_SIZE)
			{
				//当前buf写不下了, 申请一个新的buf
				if (!_push_data_buf())
				{
					return;
				}
			}

			//write str to buf
			{
				int len;

				char* buf = m_cur_buf->buf + m_cur_buf->size + LOG_ITEM_SIZE - 1;
#if defined(_MSC_VER)
				len = vsprintf_s(buf, max_len, format, ap);
#elif defined(__GNUC__)
				len = vsnprintf(buf, max_len, format, ap);
#else
#pragma error "unknow platform!!!"
#endif
				if (len < 0)
				{
					return;
				}

				wlog_item log_item;
				log_item.timestamp = wtime_api::get_time32();
				log_item.level = level;
				log_item.size = len;

				memcpy(m_cur_buf->buf + m_cur_buf->size, &log_item, LOG_ITEM_SIZE - 1);
				m_cur_buf->size += (LOG_ITEM_SIZE - 1);

				//memcpy(m_cur_buf->buf + m_cur_buf->size, str, len);
				m_cur_buf->size += len;
			}
		}
		{
			m_condition.notify_one();
		}
	}

	void wasync_log::_work_thread()
	{
		wlog_buf* log_buf = NULL;
		const wlog_item* item_ptr = NULL;
		while (true)
		{
			if (!log_buf)
			{
				std::unique_lock<std::mutex> lock{ m_lock };
				m_condition.wait(lock, [this]() { return m_data_head || m_cur_buf->size > 0 || m_stop; });

				log_buf = _get_next_data_buf();
			}

			if (!log_buf)
			{
				break;
			}

			for (unsigned int i = 0; i < log_buf->size;)
			{
				item_ptr = (const wlog_item*)(log_buf->buf + i);
				if (item_ptr->level <= m_level)
				{
					_handler_log_item(item_ptr);
				}
				i += (item_ptr->size + LOG_ITEM_SIZE - 1);
			}

			log_buf->size = 0;
			log_buf->buf[0] = 0;

			{
				std::lock_guard<std::mutex> lock{ m_lock };
				if (m_avail_head && m_avail_head->next)
				{
					//两个可用的, 这个释放
					delete log_buf;
					log_buf = NULL;
				}
				else
				{
					log_buf->next = m_avail_head;
					m_avail_head = log_buf;
				}
				log_buf = _get_next_data_buf();
			}
			if (!log_buf)
			{
				if (m_file.is_open())
				{
					m_file.flush();
				}
				if (m_show_screen)
				{
					std::cout.flush();
				}
			}
		}
	}

	wlog_buf* wasync_log::_get_next_data_buf()
	{
		wlog_buf* buf = NULL;
		if (m_data_head)
		{
			buf = m_data_head;
			m_data_head = m_data_head->next;
			if (!m_data_head)
			{
				m_data_tail = NULL;
			}
			buf->next = NULL;
		}
		else if (m_cur_buf->size > 0)
		{
			buf = m_cur_buf;
			m_cur_buf = m_avail_head;
			m_avail_head = m_avail_head->next;
			m_cur_buf->next = NULL;
		}
		return buf;
	}

	bool wasync_log::_push_data_buf()
	{
		//当前buf写不下了, 申请一个新的buf
		if (!m_avail_head)
		{
			//没有可用的，重新申请
			wlog_buf* p = new wlog_buf();
			if (!p)
			{
				return false;
			}
			m_avail_head = p;
		}

		//当前buf已满 加入链表
		if (m_data_tail)
		{
			m_data_tail->next = m_cur_buf;
			m_data_tail = m_cur_buf;
		}
		else
		{
			m_data_head = m_data_tail = m_cur_buf;
		}

		//从可用链表取一个
		m_cur_buf = m_avail_head;
		m_avail_head = m_avail_head->next;

		//初始化
		m_cur_buf->next = NULL;

		return true;
	}

	void wasync_log::_handler_log_item(const wlog_item* log_item_ptr)
	{
		if (log_item_ptr->level > m_level)
		{
			return;
		}
		if (m_timestamp > 0 && log_item_ptr->timestamp >= m_timestamp)
		{
			//更换文件
			if (m_file.is_open())
			{
				m_file.flush();
				m_file.close();
			}

			char filename[1024];
			gen_log_file_name(filename, m_path, m_name, m_ext, ELogName_AutoDate, m_timestamp);

			m_file.open(filename, std::ios_base::app);
		}
		if (m_show_screen && m_color_ptr)
		{
			m_color_ptr->set_color(g_log_name_colors[log_item_ptr->level].color);
		}
		if (log_item_ptr->level)
		{
			//添加时间前缀
			char dateTime[ASCII_DATETIME_LEN];
			wtime_api::time32_datetime_format(log_item_ptr->timestamp, dateTime, '-', ' ', ':');

			if (m_file.is_open())
			{
				m_file << '[' << dateTime << "] [" << g_log_name_colors[log_item_ptr->level].name << "] ";
			}
			if (m_show_screen)
			{
				std::cout << '[' << dateTime << "] [" << g_log_name_colors[log_item_ptr->level].name << "] ";
			}
		}
		if (m_file.is_open())
		{
			m_file.write(log_item_ptr->buf, log_item_ptr->size);
			m_file.write("\n", 1);
		}
		if (m_show_screen)
		{
			std::cout.write(log_item_ptr->buf, log_item_ptr->size);
			std::cout.write("\n", 1);
			if (m_color_ptr)
			{
				m_color_ptr->set_color(ELCT_White);
			}
		}
	}

	wasync_log::wasync_log(const wasync_log &async_log)
	{
		*this = async_log;
	}

	wasync_log& wasync_log::operator=(const wasync_log &async_log)
	{
		m_path = async_log.m_path;
		m_name = async_log.m_name;
		m_ext = async_log.m_ext;
		m_fullname = async_log.m_fullname;

		m_show_screen = async_log.m_show_screen;
		m_level = async_log.m_level;
		m_timestamp = async_log.m_timestamp;

		//m_file = async_log.m_file;

		//m_thread = async_log.m_thread;
		//m_lock = async_log.m_lock;
		//m_condition = async_log.m_condition;
		
		m_stop = async_log.m_stop;

		m_cur_buf =	async_log.m_cur_buf;
		m_avail_head = async_log.m_avail_head;
		m_data_head = async_log.m_data_head;
		m_data_tail = async_log.m_data_tail;

		m_color_ptr = async_log.m_color_ptr;
		return *this;
	}

} // namespace wang