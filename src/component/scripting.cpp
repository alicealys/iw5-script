#include <stdinc.hpp>
#include "loader/component_loader.hpp"
#include "scheduler.hpp"

#include "game/scripting/event.hpp"
#include "game/scripting/execution.hpp"
#include "game/scripting/functions.hpp"

#include "game/scripting/lua/engine.hpp"
#include "game/scripting/lua/value_conversion.hpp"

#include "scripting.hpp"

namespace scripting
{
	std::unordered_map<int, std::unordered_map<std::string, int>> fields_table;
	std::unordered_map<std::string, std::unordered_map<std::string, unsigned int>> script_function_table;
	std::unordered_map<unsigned, std::vector<unsigned>> file_map;

	namespace
	{
		utils::hook::detour vm_notify_hook;
		utils::hook::detour scr_add_class_field_hook;

		utils::hook::detour scr_load_level_hook;
		utils::hook::detour g_shutdown_game_hook;

		utils::hook::detour scr_emit_function_hook;

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

				if (e.name == "connected")
				{
					scripting::clear_entity_fields(e.entity);
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

		unsigned int function_pos(unsigned int filename, unsigned int name)
		{
			const auto scripts_pos = *reinterpret_cast<int*>(0x1D6EB14);

			const auto v2 = game::FindVariable(scripts_pos, filename);
			const auto v3 = game::FindObject(scripts_pos, v2);
			const auto v4 = game::FindVariable(v3, name);

			if (!v2 || !v3 || !v4)
			{
				return 0;
			}

			return utils::hook::invoke<unsigned int>(0x5659C0, v3, v4);
		}

		void scr_emit_function_stub(unsigned int filename, unsigned int threadName, unsigned int codePos)
		{
			if (file_map.find(filename) == file_map.end())
			{
				file_map[filename] = {};
			}

			file_map[filename].push_back(threadName);

			scr_emit_function_hook.invoke<void>(filename, threadName, codePos);
		}

		void scr_load_script_stub()
		{
			utils::hook::invoke<void>(0x523DA0);

			auto count = 0;

			for (const auto& file : file_map)
			{
				const auto* name = game::SL_ConvertToString(file.first);
				const auto filename_id = atoi(name);

				for (const auto& function : file.second)
				{
					const auto pos = function_pos(file.first, function);
					const auto function_name = scripting::find_token(function);
					const auto filename = scripting::file_list[filename_id];

					if (pos && !filename.empty() && !function_name.empty())
					{
						script_function_table[filename][function_name] = pos;
					}
				}
			}
		}
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			scr_load_level_hook.create(0x527AF0, scr_load_level_stub);
			g_shutdown_game_hook.create(0x50C100, g_shutdown_game_stub);

			scr_add_class_field_hook.create(0x567CD0, scr_add_class_field_stub);
			vm_notify_hook.create(0x569720, vm_notify_stub);

			scr_emit_function_hook.create(0x561400, scr_emit_function_stub);

			utils::hook::call(0x523F3E, scr_load_script_stub);

			scheduler::loop([]()
			{
				lua::engine::run_frame();
			});
		}
	};
}

REGISTER_COMPONENT(scripting::component)