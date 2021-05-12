#pragma once

namespace notifies
{
	extern std::vector<sol::protected_function> vm_execute_hooks;

	void add_player_damage_callback(const sol::protected_function& callback);
	void add_player_killed_callback(const sol::protected_function& callback);

	void clear_callbacks();
}