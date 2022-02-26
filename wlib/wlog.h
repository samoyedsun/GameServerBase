#ifndef W_LOG_H
#define W_LOG_H

#include <string>
#include "wlog_define.h"

namespace wang
{
	class wlog
	{
	public:
		enum { EBuf_Size = 1024 };

	public:
		explicit wlog();
		explicit wlog(ELogLevelType level);
		explicit wlog(ELogLevelType level, const char* func, int line);
		~wlog();

		static bool init(const std::string& path, const std::string& name, const std::string& ext = ".log"
			, ELogNameType name_type = ELogName_DateTime, bool mod_append = false, bool show_screen = true);
		static void destroy();

		static void fix_log(ELogLevelType level, const void* p, int len);
		static void var_log(ELogLevelType level, const char* format, ...);

		static const std::string& get_path();
		static const std::string& get_fullname();

		static void set_level(ELogLevelType level);

	public:
		wlog& operator<<(bool);
		wlog& operator<<(char);

		wlog& operator<<(signed char);
		wlog& operator<<(unsigned char);

		wlog& operator<<(signed short);
		wlog& operator<<(unsigned short);

		wlog& operator<<(signed int);
		wlog& operator<<(unsigned int);

		wlog& operator<<(signed long);
		wlog& operator<<(unsigned long);

		wlog& operator<<(signed long long);
		wlog& operator<<(unsigned long long);

		wlog& operator<<(const char *);
		wlog& operator<<(const std::string&);

		wlog& operator<<(float);
		wlog& operator<<(double);

	private:
		wlog(const wlog&);
		wlog& operator=(const wlog&);

	private:
		char m_data[EBuf_Size];
		int  m_len;
		ELogLevelType m_level;
	};

#define LOG wlog

#define SLOG LOG()

#if defined(_MSC_VER)
#define FUNCTION __FUNCTION__

#elif defined(__GNUC__)
#define FUNCTION __PRETTY_FUNCTION__

#else
#pragma error "unknow platform!!!"

#endif

	//标准log 有时间前缀
#define LOG_SYSTEM LOG(ELogLevel_System)
#define LOG_FATAL  LOG(ELogLevel_Fatal, FUNCTION, __LINE__)
#define LOG_ERROR  LOG(ELogLevel_Error, FUNCTION, __LINE__)
#define LOG_WARN   LOG(ELogLevel_Warn, FUNCTION, __LINE__)
#define LOG_INFO   LOG(ELogLevel_Info)
#define LOG_DEBUG  LOG(ELogLevel_Debug)

#define FIX_LOG LOG::fix_log

#define VAR_LOG LOG::var_log
#define NORMAL_LOG(format, ...)		VAR_LOG(ELogLevel_System, format, ##__VA_ARGS__)
#define ERROR_LOG(format, ...)		VAR_LOG(ELogLevel_Error,  format, ##__VA_ARGS__)
#define WARNING_LOG(format, ...)	VAR_LOG(ELogLevel_Warn,   format, ##__VA_ARGS__)
#define SYSTEM_LOG(format, ...)		VAR_LOG(ELogLevel_Info,   format, ##__VA_ARGS__)
#define DEBUG_LOG(format, ...)		VAR_LOG(ELogLevel_Debug,  format, ##__VA_ARGS__)

#define ERROR_EX_LOG(format, ...)	ERROR_LOG  ("[%s][%d]" format, FUNCTION, __LINE__, ##__VA_ARGS__)
#define WARNING_EX_LOG(format, ...)	WARNING_LOG("[%s][%d]" format, FUNCTION, __LINE__, ##__VA_ARGS__)

}

#endif  // W_LOG_H_
