#pragma once

namespace notifies
{
	extern std::unordered_map<unsigned, sol::protected_function> vm_execute_hooks;
	extern unsigned int function_count;
	extern bool hook_enabled;

	void add_player_damage_callback(const sol::protected_function& callback);
	void add_player_killed_callback(const sol::protected_function& callback);

	void clear_callbacks();
}