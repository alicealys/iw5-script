#include <stdinc.hpp>
#include "functions.hpp"

#include <utils/string.hpp>

namespace scripting
{
	namespace
	{
		std::unordered_map<std::string, unsigned> lowercase_map(
			const std::unordered_map<std::string, unsigned>& old_map)
		{
			std::unordered_map<std::string, unsigned> new_map{};
			for (auto& entry : old_map)
			{
				new_map[utils::string::to_lower(entry.first)] = entry.second;
			}

			return new_map;
		}

		const std::unordered_map<std::string, unsigned>& get_methods()
		{
			static auto methods = lowercase_map(method_map);
			return methods;
		}

		const std::unordered_map<std::string, unsigned>& get_functions()
		{
			static auto function = lowercase_map(function_map);
			return function;
		}

		int find_function_index(const std::string& name, const bool prefer_global)
		{
			const auto target = utils::string::to_lower(name);

			const auto& primary_map = prefer_global
				                          ? get_functions()
				                          : get_methods();
			const auto& secondary_map = !prefer_global
				                            ? get_functions()
				                            : get_methods();

			auto function_entry = primary_map.find(target);
			if (function_entry != primary_map.end())
			{
				return function_entry->second;
			}

			function_entry = secondary_map.find(target);
			if (function_entry != secondary_map.end())
			{
				return function_entry->second;
			}

			return -1;
		}

		script_function get_function_by_index(const unsigned index)
		{
			// Get plutonium's method call function
			static const auto ptr = *reinterpret_cast<int*>(0x56CBDC + 0x1) + 0x56CBDC + 0x5;
			// Get plutonium's custom function table
			static const auto function_table = *reinterpret_cast<int*>(0x56C8EB + 0x3);
			// Method table is at (method call function) + 0xA (0x6 + 0x4)
			static const auto method_table = *reinterpret_cast<int*>(ptr + 0xA);

			if (index < 0x1C7)
			{
				return reinterpret_cast<script_function*>(function_table)[index];
			}

			return reinterpret_cast<script_function*>(method_table)[index - 0x8000];
		}
	}

	std::string find_token(unsigned int id)
	{
		for (const auto& token : token_map)
		{
			if (token.second == id)
			{
				return token.first;
			}
		}

		return utils::string::va("_ID%i", id);
	}

	int find_token_id(const std::string& name)
	{
		const auto result = token_map.find(name);

		if (result != token_map.end())
		{
			return result->second;
		}

		return -1;
	}

	script_function find_function(const std::string& name, const bool prefer_global)
	{
		const auto index = find_function_index(name, prefer_global);
		if (index < 0) return nullptr;

		return get_function_by_index(index);
	}
}
