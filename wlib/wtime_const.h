#ifndef W_TIME_CONST_H
#define W_TIME_CONST_H

namespace wang {

	// 时间常量
	enum ETimeConstant
	{
		ETC_Minute = 60,					// 分钟, 单位1s
		ETC_QuarterHour = 15 * ETC_Minute,		// 一刻钟, 单位1s
		ETC_HalfHour = 30 * ETC_Minute,		// 半小时, 单位1s
		ETC_Hour = 60 * ETC_Minute,		// 小时, 单位1s
		ETC_Day = 24 * ETC_Hour,		// 天, 单位1s
		ETC_Week = 7 * ETC_Day,			// 周, 单位s
		ETC_Year = 365 * ETC_Day,		// 年, 单位s
	};

	static const int ASCII_DATETIME_LEN = 20; //2016-04-24 14:06:59
	static const int BASE_TIME = 946598400; //1900 -> 2000-1-1 的秒数

} // namespace wang

#endif // W_TIME_CONST_H
