#ifndef W_TIME_API_H
#define W_TIME_API_H

#include <ctime>

namespace wang {

	// 注意并发任务访问时的线程安全
	namespace wtime_base_api
	{
		void set_time_zone(int value);
		void set_time_adjust(int value);

		long long get_time_ms();

		time_t get_gmt();

		void time_t_to_tm(time_t time, tm& out);
		tm time_t_to_tm(time_t time);

		void get_tm(tm& out);
		tm get_tm();

		// yyyy-MM-dd HH:mm:ss
		int time64_datetime_format(const tm& now_tm, char* out, char date_conn, char datetime_conn, char time_conn);
		int time64_datetime_format(time_t time, char* out, char date_conn, char datetime_conn, char time_conn);
		int datetime_format(char* out, char date_conn, char datetime_conn, char time_conn);

		time_t get_day_stamp_time64(time_t now, int hour, int minute);
		time_t get_day_stamp_time64(int hour, int minute);

		time_t get_week_stamp_time64(time_t now, int wday, int hour, int minute);
		time_t get_week_stamp_time64(int wday, int hour, int minute);

		time_t get_this_week_stamp_time64(time_t now, int wday, int hour, int minute);
		time_t get_this_week_stamp_time64(int wday, int hour, int minute);

		time_t get_today_stamp_time64(int hour);
	};

	namespace wtime_32_api
	{
		int time_t_to_time32(time_t time);
		time_t time32_to_time_t(int time);

		int get_time32();

		void time32_to_tm(int time, tm& out);
		tm time32_to_tm(int time);

		// yyyy-MM-dd HH:mm:ss
		int time32_datetime_format(int time, char* out, char date_conn, char datetime_conn, char time_conn);

		int get_day_stamp(int now, int hour, int minute);
		int get_day_stamp(int hour, int minute);

		int get_week_stamp(int now, int wday, int hour, int minute);
		int get_week_stamp(int wday, int hour, int minute);

		int get_this_week_stamp(int now, int wday, int hour, int minute);
		int get_this_week_stamp(int wday, int hour, int minute);

		int get_today_stamp_time32(int hour);
	};

	namespace wtime_api
	{
		using namespace wtime_base_api;
		using namespace wtime_32_api;
	}
} // namespace wang

#endif // W_TIME_API_H
