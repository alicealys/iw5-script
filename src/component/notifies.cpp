#include <stdinc.hpp>

#include "game/scripting/execution.hpp"
#include "scripting.hpp"
#include "game/scripting/lua/engine.hpp"

namespace notifies
{
	namespace
	{
		utils::hook::detour client_command_hook;

		void client_command_stub(int clientNum)
		{
			char cmd[1024] = { 0 };

			game::SV_Cmd_ArgvBuffer(0, cmd, 1024);

			if (cmd == "say"s)
			{
				const auto message = game::ConcatArgs(1);

				scheduler::once([message, clientNum]()
				{
					const scripting::entity level{*game::levelEntityId};
					const auto player = scripting::call("getEntByNum", {clientNum}).as<scripting::entity>();

					scripting::notify(level, "say", {player, message});
					scripting::notify(player, "say", {message});
				});
			}

			return client_command_hook.invoke<void>(clientNum);
		}
	}

	void init()
	{
		client_command_hook.create(0x502CB0, client_command_stub);
	}
}