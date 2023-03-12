#pragma once

namespace notifies
{
	extern std::unordered_map<unsigned, sol::protected_function> vm_execute_hooks;
	extern unsigned int function_count;
	extern bool hook_enabled;

	void clear_cmd_notifies(const scripting::entity& entity);
	void clear_all_cmd_notifies();
	void add_cmd_notify(int clientNum, const std::string& cmd, const std::string& notify);

	void add_player_damage_callback(const sol::protected_function& callback);
	void add_player_killed_callback(const sol::protected_function& callback);
	void add_player_say_callback(const sol::protected_function& callback);

	void clear_callbacks();
}