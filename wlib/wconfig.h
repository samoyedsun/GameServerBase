#ifndef W_CONFIG_H
#define W_CONFIG_H

#include "wnet_type.h"
#include <string>
#include <boost/unordered_map.hpp>

namespace wang {

	class wconfig_file;

	class wconfig
	{
	public:
		wconfig();
		virtual ~wconfig();

	protected:
		bool init(uint16 values_count);
		bool open_file(const std::string& str);
		void close_file();
		void destroy();

	public:
		int32 get_int32(uint16 index) const
		{
			return _valid_index(index) ? m_configs[index].m_int32 : 0;
		}

		uint32 get_uint32(uint16 index) const
		{
			return _valid_index(index) ? m_configs[index].m_uint32 : 0;
		}

		const std::string& get_string(uint16 index) const;

	public:
		void set_int32(uint16 index, int32 value)
		{
			if (!_valid_index(index)) return;
			m_configs[index].m_int32 = value;
		}

		void set_uint32(uint16 index, uint32 value)
		{
			if (!_valid_index(index)) return;
			m_configs[index].m_uint32 = value;
		}

		void set_string(uint16 index, const std::string& value)
		{
			if (!_valid_index(index)) return;
			m_configs[index].m_str = value;
		}

		void show_config(const std::string& name, std::string* str_ptr = NULL);
		bool set_config(const std::string& name, const std::string& value);

	protected:
		bool set_int32(uint16 index, const std::string& name, int32 _default, int32 scale = 1);
		bool set_uint32(uint16 index, const std::string& name, uint32 _default, uint32 scale = 1);
		bool set_string(uint16 index, const std::string& name, const std::string& _default);
		bool set_path(uint16 index, const std::string& name, const std::string& _default);

	private:
		bool _valid_index(uint16 index) const { if (index < m_values_count) return true; _log_index(index); return false; }
		void _log_index(uint16 index) const;
		bool _can_set(uint16 index) const;

	private:
		enum EValueType
		{
			EValueTypeNull = 0,
			EValueTypeInt32,
			EValueTypeUint32,
			EValueTypeString,
		};
		struct wnode
		{
			std::string m_name;
			EValueType	m_type;
			union
			{
				int32_t		m_int32;
				uint32_t	m_uint32;
			};
			std::string m_str;

			wnode() : m_type(EValueTypeNull), m_uint32(0) {}
			void init(const std::string& name, EValueType type)
			{
				m_name = name;
				m_type = type;
			}
		};

	private:
		wnode*		m_configs;
		uint16		m_values_count;
		wconfig_file* m_cfg_file_ptr;
	};

	class wconfig_file
	{
	public:
		wconfig_file();
		~wconfig_file();

		bool open(const std::string& file_name);
		void close();

		// utf8±àÂë
		const std::string& get_string_default(const std::string& name, const std::string& _default) const;
		int32 get_int32_default(const std::string& name, const int32 _default) const;
		uint32 get_uint32_default(const std::string& name, const uint32 _default) const;

	private:
		const std::string* get_value(const std::string& key) const;
		bool add_key_pair(const std::string& key, const std::string& value);
		std::string _trim(const std::string& s);

	private:
		typedef boost::unordered_map<std::string, std::string>	wconfig_pairs;

		wconfig_pairs			m_config_pairs;
	};

} // namespace wang

#endif // _NSHARED_NCONFIG_H
