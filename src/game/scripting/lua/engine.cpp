#include <stdinc.hpp>
#include "engine.hpp"
#include "context.hpp"

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

		void load_scripts()
		{
			const auto script_dir = "scripts/"s;

			const auto path = get_path();

			std::filesystem::current_path(path);

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
		load_scripts();
	}

	void stop()
	{
		get_scripts().clear();
	}

	void notify(const event& e)
	{
		for (auto& script : get_scripts())
		{
			script->notify(e);
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
