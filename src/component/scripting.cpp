#include <stdinc.hpp>

#include "game/scripting/event.hpp"
#include "game/scripting/execution.hpp"
#include "game/scripting/lua/engine.hpp"

namespace scripting
{
	std::unordered_map<int, std::unordered_map<std::string, int>> fields_table;

	namespace
	{
		utils::hook::detour vm_notify_hook;
		utils::hook::detour scr_add_class_field_hook;

		utils::hook::detour scr_load_level_hook;
		utils::hook::detour g_shutdown_game_hook;

		void vm_notify_stub(const unsigned int notify_list_owner_id, const unsigned int string_value,
			                game::VariableValue* top)
		{
			const auto* name = game::SL_ConvertToString(string_value);

			if (name)
			{
				event e;
				e.name = name;
				e.entity = notify_list_owner_id;

				for (auto* value = top; value->type != game::SCRIPT_END; --value)
				{
					e.arguments.emplace_back(*value);
				}

				lua::engine::notify(e);
			}

			vm_notify_hook.invoke<void>(notify_list_owner_id, string_value, top);
		}

		void scr_add_class_field_stub(unsigned int classnum, unsigned int _name, unsigned int canonicalString, unsigned int offset)
		{
			const auto name = game::SL_ConvertToString(_name);

			if (fields_table[classnum].find(name) == fields_table[classnum].end())
			{
				fields_table[classnum][name] = offset;
			}

			scr_add_class_field_hook.invoke<void>(classnum, _name, canonicalString, offset);
		}

		void scr_load_level_stub()
		{
			scr_load_level_hook.invoke<void>();
			lua::engine::start();
		}

		void g_shutdown_game_stub(const int free_scripts)
		{
			lua::engine::stop();
			g_shutdown_game_hook.invoke<void>(free_scripts);
		}
	}

	void init()
	{
		scr_load_level_hook.create(0x527AF0, scr_load_level_stub);
		g_shutdown_game_hook.create(0x50C100, g_shutdown_game_stub);

		scr_add_class_field_hook.create(0x567CD0, scr_add_class_field_stub);
		vm_notify_hook.create(0x569720, vm_notify_stub);

		scheduler::loop([]()
		{
			lua::engine::run_frame();
		});
	}
}