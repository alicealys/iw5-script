#include <stdinc.hpp>

#include "game/scripting/execution.hpp"
#include "scripting.hpp"
#include "game/scripting/lua/engine.hpp"

namespace notifies
{
	namespace
	{
		void g_say_stub(game::gentity_s* ent, game::gentity_s* target, int mode, const char* chatText)
		{
			const std::string message = ++chatText;
			const auto client = ent->entnum;

			scheduler::once([ent, message, client]()
			{
				const scripting::entity level{*game::levelEntityId};
				const auto player = scripting::call("getEntByNum", {client}).as<scripting::entity>();

				scripting::notify(level, "say", {player, message});
				scripting::notify(player, "say", {message});
			});

			reinterpret_cast<void (*)(void*, void*, int, const char*)>(0x502B60)(ent, target, mode, chatText);
		}
	}

	void init()
	{
		utils::hook::call(0x502D49, g_say_stub);
	}
}