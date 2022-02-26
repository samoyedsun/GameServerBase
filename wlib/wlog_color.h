/********************************************************************
created:	2016/03/27

author:		H.B. Wang

purpose:	system log color
*********************************************************************/

#ifndef W_LOG_COLOR_H
#define W_LOG_COLOR_H

namespace wang
{
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
}
#endif