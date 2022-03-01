#ifndef W_CMD_PARAM_H
#define W_CMD_PARAM_H

#include <boost/lexical_cast.hpp>
#include <string>
#include <vector>

namespace wang {

	class wcmd_param
	{
	public:
		wcmd_param() : m_index(-1) {}
		~wcmd_param() {}

		void parse_cmd(const std::string& code);

		void set_cmd(const std::string& cmd) { m_cmd = cmd; }

		const std::string& get_cmd() const { return m_cmd; }

		void push_param(const std::string& tokens) { m_tokens.push_back(tokens); }

		int get_param_num() const { return static_cast<int>(m_tokens.size()); }

		void get_all_params(std::string& cmd) const;

		void get_params(std::string& cmd, int start_index) const;

		void get_remain_paras(std::string& cmd) const;

		template<typename T>
		bool get_param(int index, T& param) const
		{
			try
			{
				if (index >= get_param_num())
				{
					return false;
				}

				param = boost::lexical_cast<T>(m_tokens[index]);
				return true;
			}
			catch (boost::bad_lexical_cast&)
			{
				return false;
			}
		}

		template<typename T>
		bool get_next_param(T& param) const
		{
			++m_index;
			return get_param(m_index, param);
		}

	private:
		wcmd_param(const wcmd_param&);
		wcmd_param& operator=(const wcmd_param&);

	private:
		std::string				m_cmd;
		std::vector<std::string> m_tokens;
		mutable int m_index;
	};

} // namespace wang

#endif // W_CMD_PARAM_H
