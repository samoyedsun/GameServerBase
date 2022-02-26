/**
*
*	Copyright (C) 2010 FastTime
*	Description: 单例模式（线程安全）模板类
*	Author: H.B. Wang
*	Date:	2016.4.20
*/

#ifndef W_SINGLETON_H
#define W_SINGLETON_H


namespace wang
{
	template < typename T >
	class wsingleton
	{
	public:
		static T& get_instance()
		{
			static T v;
			return v;
		}
	};
}

#endif // _W_SINGLETON_H_
