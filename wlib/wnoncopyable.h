#ifndef W_NONCOPYABLE_H
#define W_NONCOPYABLE_H

namespace wang
{
	class wnoncopyable
	{
	protected:
		wnoncopyable() {}
		~wnoncopyable() {}

	private:
		wnoncopyable(const wnoncopyable&);
		wnoncopyable& operator=(const wnoncopyable&);
	};

}
#endif