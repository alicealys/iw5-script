#include <stdinc.hpp>

#include "game/scripting/execution.hpp"
#include "scripting.hpp"

namespace notifies
{
	namespace
	{
		/*auto hidden = true;

		int g_say_to;
		int pre_say;

		char* evaluate_say(char* text, game::gentity_s* ent)
		{
			hidden = false;

			const std::string message = text;
			const auto client = ent->entity_num;

			scheduler::once([message, client]()
			{
				const scripting::entity level{*game::levelEntityId};
				const auto player = scripting::call("getEntByNum", {client}).as<scripting::entity>();

				scripting::notify(level, "say", {player, message});
				scripting::notify(player, "say", {message});
			});

			if (text[0] == '/')
			{
				hidden = true;
				++text;
			}

			return text;
		}

		char* clean_str(const char* str)
		{
			return game::I_CleanStr(str);
		}

		__declspec(naked) void pre_say_stub()
		{
			__asm
			{
				mov eax, [esp + 0xE4 + 0x10]

				push eax
				pushad

				push[esp + 0xE4 + 0x28]
				push eax
				call evaluate_say
				add esp, 0x8

				mov[esp + 0x20], eax
				popad
				pop eax

				mov[esp + 0xE4 + 0x10], eax

				call clean_str

				push pre_say
				retn
			}
		}

		__declspec(naked) void post_say_stub()
		{
			__asm
			{
				push eax

				xor eax, eax

				mov al, hidden

				cmp al, 0
				jne hide

				pop eax

				push g_say_to
				retn
			hide:
				pop eax

				retn
			}
		}*/
	}

	void init()
	{
		/*g_say_to = SELECT(0x82BB50, 0x82A3D0);
		pre_say = SELECT(0x6A7AB3, 0x493E63);

		utils::hook::jump(SELECT(0x6A7AAE, 0x493E5E), pre_say_stub);
		utils::hook::call(SELECT(0x6A7B5F, 0x493F0F), post_say_stub);
		utils::hook::call(SELECT(0x6A7B9B, 0x493F4B), post_say_stub);*/

		//utils::hook::nop(SELECT(0x45A32B, 0x4B8D6B), 7);
	}
}