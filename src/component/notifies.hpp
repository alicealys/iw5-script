#pragma once

namespace notifies
{
	void add_player_damage_callback(const sol::protected_function& callback);
	void add_player_killed_callback(const sol::protected_function& callback);

	void clear_callbacks();
}