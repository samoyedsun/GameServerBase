#ifndef W_LOG_DEFINE_H
#define W_LOG_DEFINE_H

namespace wang
{
	enum ELogLevelType
	{
		ELogLevel_None = 0,
		ELogLevel_System,
		ELogLevel_Fatal,
		ELogLevel_Error,
		ELogLevel_Warn,
		ELogLevel_Info,
		ELogLevel_Debug,
		ELogLevel_Num,
	};

	enum ELogNameType
	{
		ELogName_None = 0,
		ELogName_Date,
		ELogName_DateTime,
		ELogName_AutoDate,
	};
}

#endif  // W_LOG_DEFINE_H
