#include <stdinc.hpp>
#include "engine.hpp"
#include "context.hpp"

#include "../../../component/notifies.hpp"
#include "../../../component/command.hpp"

#include <utils/io.hpp>

namespace scripting::lua::engine
{
	namespace
	{
		auto& get_scripts()
		{
			static std::vector<std::unique_ptr<context>> scripts{};
			return scripts;
		}

		std::string load_path()
		{
			const auto fs_basegame = game::Dvar_FindVar("fs_basegame");
			return fs_basegame->current.string;
		}

		std::string get_path()
		{
			static const auto path = load_path();
			return path;
		}

		void load_scripts(const std::string& script_dir)
		{
			const auto path = get_path();
			std::filesystem::current_path(path);

			if (!utils::io::directory_exists(script_dir))
			{
				return;
			}

			const auto scripts = utils::io::list_files(script_dir);

			for (const auto& script : scripts)
			{
				if (std::filesystem::is_directory(script) && utils::io::file_exists(script + "/__init__.lua"))
				{
					get_scripts().push_back(std::make_unique<context>(script));
				}
			}
		}
	}

	void start()
	{
		get_scripts().clear();
		load_scripts("scripts/");
	}

	void stop()
	{
		command::clear_script_commands();
		notifies::clear_callbacks();
		get_scripts().clear();
	}

	void notify(const event& e)
	{
		for (auto& script : get_scripts())
		{
			script->notify(e);
		}
	}

	void handle_endon_conditions(const event& e)
	{
		for (auto& script : get_scripts())
		{
			script->handle_endon_conditions(e);
		}
	}

	void run_frame()
	{
		for (auto& script : get_scripts())
		{
			script->run_frame();
		}
	}
}
