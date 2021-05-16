#pragma once
#include "game/game.hpp"

namespace scripting
{
	extern std::unordered_map<std::string, unsigned> method_map;
	extern std::unordered_map<std::string, unsigned> function_map;
	extern std::unordered_map<std::string, unsigned> token_map;
	extern std::unordered_map<unsigned, std::string> file_list;

	using script_function = void(*)(game::scr_entref_t);

	script_function find_function(const std::string& name, const bool prefer_global);
	int find_token_id(const std::string& name);
	std::string find_token(unsigned int id);
}
