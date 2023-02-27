#include <stdinc.hpp>
#include "loader/component_loader.hpp"
#include "scheduler.hpp"

#include "game/scripting/event.hpp"
#include "game/scripting/execution.hpp"
#include "game/scripting/functions.hpp"

#include "game/scripting/lua/engine.hpp"
#include "game/scripting/lua/value_conversion.hpp"

#include "scripting.hpp"
#include "notifies.hpp"

namespace scripting
{
	std::unordered_map<int, std::unordered_map<std::string, int>> fields_table;
	std::unordered_map<std::string, std::unordered_map<std::string, unsigned int>> script_function_table;

	namespace
	{
		utils::hook::detour vm_notify_hook;
		utils::hook::detour scr_add_class_field_hook;

		utils::hook::detour scr_load_level_hook;
		utils::hook::detour g_shutdown_game_hook;

		utils::hook::detour scr_set_thread_position_hook;
		utils::hook::detour process_script_hook;

		utils::hook::detour scr_run_current_threads_hook;

		std::string current_file;
		
		using notify_list = std::vector<event>;
		utils::concurrency::container<notify_list> scheduled_notifies;

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

				if (e.name == "entitydeleted")
				{
					const auto entity = e.arguments[0].as<scripting::entity>();

					notifies::clear_cmd_notifies(entity);
				}

				lua::engine::handle_endon_conditions(e);

				scheduled_notifies.access([&](notify_list& list)
				{
					list.push_back(e);
				});
			}

			vm_notify_hook.invoke<void>(notify_list_owner_id, string_value, top);
		}

		void clear_scheduled_notifies()
		{
			scheduled_notifies.access([](notify_list& list)
			{
				list.clear();
			});
		}

		void scr_add_class_field_stub(unsigned int classnum, unsigned int name, 
			unsigned int canonicalString, unsigned int offset)
		{
			const auto name_str = game::SL_ConvertToString(name);

			if (fields_table[classnum].find(name_str) == fields_table[classnum].end())
			{
				fields_table[classnum][name_str] = offset;
			}

			scr_add_class_field_hook.invoke<void>(classnum, name, canonicalString, offset);
		}

		void scr_load_level_stub()
		{
			scr_load_level_hook.invoke<void>();
			clear_scheduled_notifies();
			lua::engine::start();
		}

		void g_shutdown_game_stub(const int free_scripts)
		{
			clear_scheduled_notifies();
			lua::engine::stop();
			g_shutdown_game_hook.invoke<void>(free_scripts);
		}

		void process_script_stub(const char* filename)
		{
			current_file = filename;

			const auto file_id = atoi(filename);
			if (file_id)
			{
				current_file = scripting::find_file(file_id);
			}

			process_script_hook.invoke<void>(filename);
		}

		void scr_set_thread_position_stub(unsigned int threadName, unsigned int codePos)
		{
			const auto function_name = scripting::find_token(threadName);

			if (!function_name.empty())
			{
				script_function_table[current_file][function_name] = codePos;
			}

			scr_set_thread_position_hook.invoke<void>(threadName, codePos);
		}

		void scr_run_current_threads_stub()
		{
			notify_list list_copy{};
			scheduled_notifies.access([&](notify_list& list)
			{
				list_copy = list;
				list.clear();
			});

			for (const auto& e : list_copy)
			{
				lua::engine::notify(e);
			}

			scr_run_current_threads_hook.invoke<void>();
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

			scr_set_thread_position_hook.create(0x5616D0, scr_set_thread_position_stub);
			process_script_hook.create(0x56B130, process_script_stub);

			scr_run_current_threads_hook.create(0x56E320, scr_run_current_threads_stub);

			scheduler::loop([]()
			{
				lua::engine::run_frame();
			});
		}
	};
}

REGISTER_COMPONENT(scripting::component)