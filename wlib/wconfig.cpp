#include "wconfig.h"
#include "wlog.h"
#include "wlib_util.h"
#include <sstream>
#include <fstream>

namespace wang {

	wconfig::wconfig() : m_configs(NULL), m_values_count(0), m_cfg_file_ptr(NULL)
	{
	}

	wconfig::~wconfig()
	{
		destroy();
	}

	bool wconfig::init(uint16 values_count)
	{
		m_configs = new wnode[values_count];

		if (!m_configs)
		{
			return false;
		}

		m_values_count = values_count;

		return true;
	}

	bool wconfig::open_file(const std::string& str)
	{
		m_cfg_file_ptr = new wconfig_file;
		if (!m_cfg_file_ptr)
		{
			return false;
		}
		if (!m_cfg_file_ptr->open(str))
		{
			LOG_ERROR << "open config file error, " << str;
			return false;
		}
		return true;
	}
	void wconfig::close_file()
	{
		if (m_cfg_file_ptr)
		{
			m_cfg_file_ptr->close();
			delete m_cfg_file_ptr;
			m_cfg_file_ptr = NULL;
		}
	}

	void wconfig::destroy()
	{
		if (m_configs)
		{
			delete[] m_configs;
			m_configs = NULL;
		}
		m_values_count = 0;

		close_file();
	}

	const std::string& wconfig::get_string(uint16 index) const
	{
		return _valid_index(index) ? m_configs[index].m_str : g_null_str;
	}

	void wconfig::show_config(const std::string& name, std::string* str_ptr)
	{
		std::stringstream ss;
		for (uint16_t i = 0; i < m_values_count; ++i)
		{
			if (!name.empty() && m_configs[i].m_name.find(name.c_str()) == std::string::npos)
			{
				continue;
			}
			ss << m_configs[i].m_name << ":";
			switch (m_configs[i].m_type)
			{
			case EValueTypeNull:
				ss << "not set";
				break;
			case EValueTypeInt32:
				ss << m_configs[i].m_int32;
				break;
			case EValueTypeUint32:
				ss << m_configs[i].m_uint32;
				break;
			case EValueTypeString:
				ss << m_configs[i].m_str;
				break;
			default:
				ss << "unknown type " << m_configs[i].m_type;
				break;
			}
		}
		if (str_ptr)
		{
			*str_ptr = ss.str();
		}
	}

	bool wconfig::set_config(const std::string& name, const std::string& value)
	{
		for (uint16_t i = 0; i < m_values_count; ++i)
		{
			if (name != m_configs[i].m_name)
			{
				continue;
			}
			LOG_INFO << "set config " << name << '=' << value;
			if (m_configs[i].m_type == EValueTypeInt32)
			{
				int32 v = atoi(value.c_str());
				set_int32(i, v);
				return true;
			}
			else if (m_configs[i].m_type == EValueTypeUint32)
			{
				uint32 v = static_cast<uint32>(strtoul(value.c_str(), NULL, 10));
				set_uint32(i, v);
				return true;
			}
			else if (m_configs[i].m_type == EValueTypeString)
			{
				set_string(i, value);
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}

	bool wconfig::set_int32(uint16 index, const std::string& name, int32 _default, int32 scale)
	{
		if (!_can_set(index))
		{
			return false;
		}
		m_configs[index].m_name = name;
		m_configs[index].m_type = EValueTypeInt32;
		m_configs[index].m_int32 = m_cfg_file_ptr->get_int32_default(name, _default) * scale;
		return true;
	}

	bool wconfig::set_uint32(uint16 index, const std::string& name, uint32 _default, uint32 scale)
	{
		if (!_can_set(index))
		{
			return false;
		}
		m_configs[index].m_name = name;
		m_configs[index].m_type = EValueTypeUint32;
		m_configs[index].m_uint32 = m_cfg_file_ptr->get_uint32_default(name, _default) * scale;
		return true;
	}

	bool wconfig::set_string(uint16 index, const std::string& name, const std::string& _default)
	{
		if (!_can_set(index))
		{
			return false;
		}
		m_configs[index].m_name = name;
		m_configs[index].m_type = EValueTypeString;
		m_configs[index].m_str = m_cfg_file_ptr->get_string_default(name, _default);
		return true;
	}

	bool wconfig::set_path(uint16 index, const std::string& name, const std::string& _default)
	{
		if (!set_string(index, name, _default))
		{
			return false;
		}
		std::string& path = m_configs[index].m_str;
		if (!path.empty() && path.back() != '\\' && path.back() != '/')
		{
			path.append(1, '/');
		}
		return true;
	}

	void wconfig::_log_index(uint16 index) const
	{
		LOG_ERROR << "invalid index " << index;
	}

	bool wconfig::_can_set(uint16 index) const
	{
		if (!_valid_index(index))
		{
			return false;
		}
		if (!m_cfg_file_ptr)
		{
			LOG_ERROR << "config file not open";
			return false;
		}
		if (m_configs[index].m_type != EValueTypeNull)
		{
			LOG_ERROR << "index " << index << "has set";
		}
		return true;
	}

	wconfig_file::wconfig_file()
	{
	}

	wconfig_file::~wconfig_file()
	{
		close();
	}

	bool wconfig_file::open(const std::string& file_name)
	{
		std::ifstream fin(file_name.c_str());

		if (!fin.is_open())
		{
			LOG_ERROR << "can not open file " << file_name;
			return false;
		}

		std::string str_line;
		std::string::size_type n;
		std::string name;
		std::string value;
		int line = 0;

		while (std::getline(fin, str_line))
		{
			++line;
			if (1 == line && str_line.length() >= 3
				&& static_cast<unsigned char>(str_line[0]) == 0xEF
				&& static_cast<unsigned char>(str_line[1]) == 0xBB
				&& static_cast<unsigned char>(str_line[2]) == 0xBF)
			{
				//utf8
				str_line.erase(str_line.begin(), str_line.begin() + 3);
			}
			// strip '#' comments and whitespace
			if ((n = str_line.find('#')) != std::string::npos)
			{
				str_line = str_line.substr(0, n);
			}

			str_line = _trim(str_line);

			if ((n = str_line.find('=')) != std::string::npos)
			{
				name = _trim(str_line.substr(0, n));
				value = _trim(str_line.substr(n + 1));

				// º”»ÎµΩplayer map
				if (!add_key_pair(name, value))
				{
					LOG_ERROR << "insert name " << name << " value error, line=" << line;;
					return false;
				}
			}
		}
		fin.close();
		return true;
	}

	void wconfig_file::close()
	{
		m_config_pairs.clear();
	}

	const std::string& wconfig_file::get_string_default(const std::string& name, const std::string& _default) const
	{
		const std::string* p = get_value(name);
		if (!p)
		{
			return _default;
		}
		return *p;
	}

	int32 wconfig_file::get_int32_default(const std::string& name, const int32 _default) const
	{
		const std::string* p = get_value(name);

		if (!p)
		{
			return _default;
		}

		return atoi(p->c_str());
	}

	uint32 wconfig_file::get_uint32_default(const std::string& name, const uint32 _default) const
	{
		const std::string* p = get_value(name);

		if (!p)
		{
			return _default;
		}

		return uint32(atoi(p->c_str()));
	}

	const std::string* wconfig_file::get_value(const std::string& key) const
	{
		wconfig_pairs::const_iterator it = m_config_pairs.find(key);
		if (it != m_config_pairs.end())
		{
			return &(it->second);
		}
		return NULL;
	}
	bool wconfig_file::add_key_pair(const std::string& key, const std::string& value)
	{
		wconfig_pairs::iterator it = m_config_pairs.find(key);
		if (it != m_config_pairs.end())
		{
			LOG_ERROR << "key " << key << " has exist";
			return false;
		}

		size_t len = value.length();
		if (len >= 2 && value.at(0) == '"' && value.at(len - 1) == '"')
		{
			std::string _value(value.substr(1, len - 2));
			if (!m_config_pairs.insert(std::make_pair(key, _value)).second)
			{
				LOG_ERROR << "insert " << key << " failed";
				return false;
			}
		}
		else
		{
			if (!m_config_pairs.insert(std::make_pair(key, value)).second)
			{
				LOG_ERROR << "insert " << key << " failed";
				return false;
			}
		}

		return true;
	}

	std::string wconfig_file::_trim(const std::string& s)
	{
		std::string::size_type n, n2;
		n = s.find_first_not_of(" \t\r\n");

		if (n == std::string::npos)
		{
			return std::string();
		}
		else
		{
			n2 = s.find_last_not_of(" \t\r\n");
			return s.substr(n, n2 - n + 1);
		}
	}
} // namespace wang
