#include "wlog.h"
#include "wasync_log.h"
#include "wtime_api.h"
#include "wdigit2str.h"
#include <cstring>
#include <stdarg.h>

namespace wang
{
	static wasync_log* g_log_ptr = nullptr;

	namespace wstream_log_util
	{
		static void append_to(char* dst, int& dst_len, const void* src, int src_len)
		{
			if (src_len <= 0 || !src) return;

			int remain = wlog::EBuf_Size - dst_len;
			if (remain >= src_len)
			{
				memcpy(dst + dst_len, src, src_len);
				dst_len += src_len;
			}
			else if (remain > 0)
			{
				memcpy(dst + dst_len, src, remain);
				dst_len += remain;
			}
		}

		template<typename T>
		static void serial_to(char* dst, int& dst_len, const T& value)
		{
			//uint64 === 20 bit
			if (dst_len + 32 <= wlog::EBuf_Size)
			{
				dst_len += digit2str_dec(dst, dst_len, value);
			}
			else
			{
				char buf[32];
				int len = digit2str_dec(buf, 32, value);
				append_to(dst, dst_len, buf, len);
			}
		}
	}

	wlog::wlog()
		: m_len(0), m_level(ELogLevel_None)
	{

	}

	wlog::wlog(ELogLevelType level)
		: m_len(0), m_level(level)
	{

	}

	wlog::wlog(ELogLevelType level, const char* func, int line)
		: m_len(0), m_level(level)
	{
		*this << '[' << func << ':' << line << "] ";
	}

	wlog::~wlog()
	{
		if (g_log_ptr && m_len > 0 && m_level <= g_log_ptr->get_level())
		{
			g_log_ptr->append_fix(m_level, m_data, m_len);
		}
	}

	bool wlog::init(const std::string& path, const std::string& name, const std::string& ext
		, ELogNameType name_type, bool mod_append, bool show_screen)
	{
		if (g_log_ptr) return true;
		g_log_ptr = new wasync_log();
		if (!g_log_ptr)
		{
			return false;
		}
		return g_log_ptr->init(path, name, ext, name_type, mod_append, show_screen);
	}

	void wlog::destroy()
	{
		if (g_log_ptr)
		{
			g_log_ptr->destroy();
			delete g_log_ptr;
			g_log_ptr = nullptr;
		}
	}

	void wlog::fix_log(ELogLevelType level, const void* p, int len)
	{
		if (g_log_ptr)
		{
			if (len <= EBuf_Size)
			{
				g_log_ptr->append_fix(level, p, len);
			}
			else
			{
				g_log_ptr->append_fix(level, p, EBuf_Size);
			}
		}
	}

	void wlog::var_log(ELogLevelType level, const char* format, ...)
	{
		if (g_log_ptr)
		{
			va_list ap;
			va_start(ap, format);
			g_log_ptr->append_var(level, format, ap);
			va_end(ap);
		}
	}

	const std::string& wlog::get_path()
	{
		return g_log_ptr->get_path();
	}
	const std::string& wlog::get_fullname()
	{
		return g_log_ptr->get_fullname();
	}

	void wlog::set_level(ELogLevelType level)
	{
		if (g_log_ptr)
		{
			g_log_ptr->set_level(level);
		}
	}

	wlog& wlog::operator<<(bool value)
	{
		if (value)
		{
			return *this << '1';
		}
		else
		{
			return *this << '0';
		}
	}

	wlog& wlog::operator<<(char value)
	{
		if (m_len < EBuf_Size)
		{
			m_data[m_len++] = value;
		}
		return *this;
	}

	wlog& wlog::operator<<(signed char value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
	wlog& wlog::operator<<(unsigned char value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}

	wlog& wlog::operator<<(signed short value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
	wlog& wlog::operator<<(unsigned short value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}

	wlog& wlog::operator<<(signed int value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
	wlog& wlog::operator<<(unsigned int value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}

	wlog& wlog::operator<<(signed long value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
	wlog& wlog::operator<<(unsigned long value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}

	wlog& wlog::operator<<(signed long long value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
	wlog& wlog::operator<<(unsigned long long value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}

	wlog& wlog::operator<<(const char * value)
	{
		wstream_log_util::append_to(m_data, m_len, value, static_cast<int>(strlen(value)));
		return *this;
	}

	wlog& wlog::operator<<(const std::string& value)
	{
		wstream_log_util::append_to(m_data, m_len, value.data(), static_cast<int>(value.length()));
		return *this;
	}

	wlog& wlog::operator<<(float value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
	wlog& wlog::operator<<(double value)
	{
		wstream_log_util::serial_to(m_data, m_len, value);
		return *this;
	}
}