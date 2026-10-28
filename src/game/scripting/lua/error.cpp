#include <stdinc.hpp>
#include "error.hpp"
#include "../execution.hpp"

namespace scripting::lua
{
	namespace
	{
		void notify_error()
		{
			try
			{
				call("iprintln", {"^1Script execution error!"});
			}
			catch (...)
			{

			}
		}
	}

	void handle_error(const sol::protected_function_result& result)
	{
		if (!result.valid())
		{
			printf("************** Script execution error **************\n");

			const sol::error err = result;
			printf("%s\n", err.what());

			printf("****************************************************\n");

			notify_error();
		}
	}
}
