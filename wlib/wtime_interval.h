#ifndef W_TIME_INTERVAL_H
#define W_TIME_INTERVAL_H

namespace wang {

	class wtime_interval
	{
	public:
		wtime_interval() : m_interval(0), m_current(0) {}

		wtime_interval(unsigned int interval) : m_interval(interval), m_current(0) {}

		void update(unsigned int elapse) { m_current += elapse; }

		bool passed() const { return m_current >= m_interval; }

		void reset() { if (0 != m_interval) { m_current %= m_interval; } }

		void set_current(unsigned int current) { m_current = current; }

		void set_interval(unsigned int interval) { m_interval = interval; }

		unsigned int get_interval() const { return m_interval; }

		unsigned int get_current() const { return m_current; }

	private:
		unsigned int	m_interval;
		unsigned int	m_current;
	};

} // namespace wang

#endif // W_TIME_INTERVAL_H