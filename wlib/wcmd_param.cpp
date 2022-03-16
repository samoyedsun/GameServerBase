#include "wcmd_param.h"

namespace wang {

		void wcmd_param::parse_cmd(const std::string& code)
		{
			std::string str;
			for (std::string::const_iterator it = code.begin(); it != code.end(); ++it)
			{
				if (*it == ' ')
				{
					if (str.empty())
						continue;
					if (m_cmd.empty())
						m_cmd = str;
					else
						m_tokens.push_back(str);
					str.clear();
				}
				else
				{
					str.append(1, *it);
				}
			}

			if (!str.empty())
			{
				if (m_cmd.empty())
					m_cmd = str;
				else
					m_tokens.push_back(str);
			}
		}

		void wcmd_param::get_all_params(std::string& cmd) const
		{
			for (std::vector<std::string>::const_iterator iter = m_tokens.begin(); iter != m_tokens.end(); ++iter)
				cmd += *iter + " ";
		}

		void wcmd_param::get_params(std::string& cmd, int start_index) const
		{
			for (int i = start_index; i < m_tokens.size(); ++i)
				cmd += m_tokens[i] + " ";
		}

		void wcmd_param::get_remain_paras(std::string& cmd) const
		{
			for (int i = m_index; i < m_tokens.size(); ++i)
				cmd += m_tokens[i] + " ";
		}
}