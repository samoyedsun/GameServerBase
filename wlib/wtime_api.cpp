#include <cstdio>
#include <chrono>
#include "wtime_api.h"
#include "wtime_const.h"
#include <iostream>
#include <boost/container/detail/singleton.hpp>

#ifdef _MSC_VER
#pragma warning (disable:4996)
#endif

namespace wang {

	namespace wtime_base_api
	{
		//UTC时间与本地时间的差值 时区
		static time_t g_time_zone = 0;

		//人为调整时间
		static long long g_time_adjust = 0;

		static const auto g_start_time = std::chrono::steady_clock::now();

		struct wtime_init
		{
			wtime_init()
			{
				// UTC
				time_t now = time(0);
				// further convert to GMT presuming now in local
				struct tm *ptmgm = gmtime(&now);
				time_t gmnow = mktime(ptmgm);
				g_time_zone = now - gmnow;
				if (ptmgm->tm_isdst > 0) {
					g_time_zone = g_time_zone - 60 * 60;
				}
				std::cout << "timezone=" << g_time_zone / ETC_Hour << std::endl;
			}
		};

		//call before main
		static const wtime_init time_init;

		void set_time_zone(int value)
		{
			if (value < -12 || value > 12) return;
			g_time_zone = value * ETC_Hour;
		}

		void set_time_adjust(int value)
		{
			g_time_adjust = value;
		}

		long long get_time_ms()
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::steady_clock::now() - g_start_time).count();
		}

		time_t get_gmt()
		{
			return (::time(NULL) + g_time_adjust);
		}

		void time_t_to_tm(time_t time, tm& out)
		{
			time += g_time_zone;
#if defined(_MSC_VER)
			::gmtime_s(&out, &time);
#elif defined(__GNUC__)
			::gmtime_r(&time, &out);
#else
#			pragma error "Unknown Platform Not Supported. Abort! Abort!"
#endif // 
		}

		tm time_t_to_tm(time_t time)
		{
			tm out;
			time_t_to_tm(time, out);
			return out;
		}

		void get_tm(tm& out)
		{
			time_t_to_tm(get_gmt(), out);
		}

		tm get_tm()
		{
			tm out;
			get_tm(out);
			return out;
		}

		// yyyy-MM-dd HH:mm:ss
		int time64_datetime_format(const tm& now_tm, char* out, char date_conn, char datetime_conn, char time_conn)
		{
			int nCount = 0;

			if (date_conn > 0)
			{
				nCount += sprintf(out + nCount, "%04d%c%02d%c%02d", now_tm.tm_year + 1900, date_conn
					, now_tm.tm_mon + 1, date_conn, now_tm.tm_mday);
			}
			else if (date_conn == 0)
			{
				nCount += sprintf(out + nCount, "%04d%02d%02d", now_tm.tm_year + 1900, now_tm.tm_mon + 1
					, now_tm.tm_mday);
			}

			if (datetime_conn > 0)
			{
				out[nCount] = datetime_conn;
				++nCount;
				out[nCount] = '\0';
			}

			if (time_conn > 0)
			{
				nCount += sprintf(out + nCount, "%02d%c%02d%c%02d", now_tm.tm_hour, time_conn
					, now_tm.tm_min, time_conn, now_tm.tm_sec);
			}
			else if (time_conn == 0)
			{
				nCount += sprintf(out + nCount, "%02d%02d%02d", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
			}
			return nCount;
		}
		int time64_datetime_format(time_t time, char* out, char date_conn, char datetime_conn, char time_conn)
		{
			tm now_tm;
			time_t_to_tm(time, now_tm);
			return time64_datetime_format(now_tm, out, date_conn, datetime_conn, time_conn);
		}
		int datetime_format(char* out, char date_conn, char datetime_conn, char time_conn)
		{
			return time64_datetime_format(get_gmt(), out, date_conn, datetime_conn, time_conn);
		}

		time_t get_day_stamp_time64(time_t now, int hour, int minute)
		{
			tm now_tm;
			time_t_to_tm(now, now_tm);

			int day = ETC_Day;
			if (now_tm.tm_hour < hour
				|| (now_tm.tm_hour == hour && now_tm.tm_min < minute)
				)
			{
				day = 0;
			}

			return  (now
				- (now_tm.tm_hour - hour) * ETC_Hour
				- (now_tm.tm_min - minute) * ETC_Minute
				- now_tm.tm_sec
				+ day
				);
		}
		time_t get_day_stamp_time64(int hour, int minute)
		{
			return get_day_stamp_time64(get_gmt(), hour, minute);
		}

		time_t get_week_stamp_time64(time_t now, int wday, int hour, int minute)
		{
			tm now_tm;
			time_t_to_tm(now, now_tm);

			int week = ETC_Week;
			if (now_tm.tm_wday < wday
				|| (now_tm.tm_wday == wday && now_tm.tm_hour < hour)
				|| (now_tm.tm_wday == wday && now_tm.tm_hour == hour && now_tm.tm_min < minute)
				)
			{
				week = 0;
			}

			return (now
				- (now_tm.tm_wday - wday)  * ETC_Day
				- (now_tm.tm_hour - hour)  * ETC_Hour
				- (now_tm.tm_min - minute) * ETC_Minute
				- now_tm.tm_sec
				+ week
				);
		}

		time_t get_week_stamp_time64(int wday, int hour, int minute)
		{
			return get_week_stamp_time64(get_gmt(), wday, hour, minute);
		}

		time_t get_this_week_stamp_time64(time_t now, int wday, int hour, int minute)
		{
			tm now_tm;
			time_t_to_tm(now, now_tm);

			return (now
				- (now_tm.tm_wday - wday)  * ETC_Day
				- (now_tm.tm_hour - hour)  * ETC_Hour
				- (now_tm.tm_min - minute) * ETC_Minute
				- now_tm.tm_sec
				);
		}

		time_t get_this_week_stamp_time64(int wday, int hour, int minute)
		{
			return get_this_week_stamp_time64(get_gmt(), wday, hour, minute);
		}

		time_t get_today_stamp_time64(int hour)
		{
			tm now_tm;
			time_t now = get_gmt();
			time_t_to_tm(now, now_tm);

			return (now
				- (now_tm.tm_hour - hour) * ETC_Hour
				- now_tm.tm_min * ETC_Minute
				- now_tm.tm_sec
				);
		}
	}

	namespace wtime_32_api
	{
		using namespace wtime_base_api;

		int time_t_to_time32(time_t time)
		{
			return static_cast<int>(time - BASE_TIME);
		}

		time_t time32_to_time_t(int time)
		{
			return BASE_TIME + time;
		}


		int get_time32()
		{
			return time_t_to_time32(get_gmt());
		}

		void time32_to_tm(int time, tm& out)
		{
			time_t_to_tm(time32_to_time_t(time), out);
		}

		tm time32_to_tm(int time)
		{
			tm out;
			time32_to_tm(time, out);
			return out;
		}

		// yyyyMMddHHmmss
		int time32_datetime_format(int time, char* out, char date_conn, char datetime_conn, char time_conn)
		{
			return time64_datetime_format(time32_to_time_t(time), out, date_conn, datetime_conn, time_conn);
		}

		int get_day_stamp(int now_t, int hour, int minute)
		{
			return time_t_to_time32(get_day_stamp_time64(time32_to_time_t(now_t), hour, minute));
		}

		int get_day_stamp(int hour, int minute)
		{
			return time_t_to_time32(get_day_stamp_time64(hour, minute));
		}

		int get_week_stamp(int now_t, int wday, int hour, int minute)
		{
			return time_t_to_time32(get_week_stamp_time64(time32_to_time_t(now_t), wday, hour, minute));
		}

		int get_week_stamp(int wday, int hour, int minute)
		{
			return time_t_to_time32(get_week_stamp_time64(wday, hour, minute));
		}

		int get_this_week_stamp(int now, int wday, int hour, int minute)
		{
			return time_t_to_time32(get_this_week_stamp_time64(time32_to_time_t(now), wday, hour, minute));
		}

		int get_this_week_stamp(int wday, int hour, int minute)
		{
			return time_t_to_time32(get_this_week_stamp_time64(wday, hour, minute));
		}

		int get_today_stamp_time32(int hour)
		{
			return time_t_to_time32(get_today_stamp_time64(hour));
		}

	}
} // namespace wang
