#include <stdinc.hpp>
#include "loader/component_loader.hpp"
#include "scheduler.hpp"

#include "game/scripting/entity.hpp"
#include "game/scripting/execution.hpp"
#include "game/scripting/lua/value_conversion.hpp"
#include "game/scripting/lua/error.hpp"
#include "notifies.hpp"

namespace notifies
{
	std::unordered_map<unsigned, sol::protected_function> vm_execute_hooks;
	unsigned int function_count = 0;
	bool hook_enabled = true;

	namespace
	{
		utils::hook::detour client_command_hook;

		utils::hook::detour scr_player_killed_hook;
		utils::hook::detour scr_player_damage_hook;

		std::vector<sol::protected_function> player_killed_callbacks;
		std::vector<sol::protected_function> player_damage_callbacks;
		std::vector<sol::protected_function> player_say_callbacks;

		std::unordered_map<int, std::vector<std::pair<int, std::string>>> cmd_notifies;

		char empty_function[2] = {0x76, 0x0}; // CHECK_CLEAR_PARAMS, END
		char* empty_function_pos = empty_function;

		sol::lua_value convert_entity(lua_State* state, const game::gentity_s* ent)
		{
			if (!ent)
			{
				return {};
			}

			const auto player = scripting::call("getEntByNum", {ent->entnum});

			return scripting::lua::convert(state, player);
		}

		std::string get_weapon_name(unsigned int weapon, bool isAlternate)
		{
			char output[1024] = { 0 };
			game::BG_GetWeaponNameComplete(weapon, isAlternate, output, 1024);

			return output;
		}

		sol::lua_value convert_vector(lua_State* state, const float* vec)
		{
			if (!vec)
			{
				return {};
			}

			const auto _vec = scripting::vector(vec);

			return scripting::lua::convert(state, _vec);
		}

		std::string convert_mod(const int mod)
		{
			switch (mod)
			{
			case game::MOD_UNKNOWN: return "MOD_UNKNOWN";
			case game::MOD_PISTOL_BULLET: return "MOD_PISTOL_BULLET";
			case game::MOD_RIFLE_BULLET: return "MOD_RIFLE_BULLET";
			case game::MOD_EXPLOSIVE_BULLET: return "MOD_EXPLOSIVE_BULLET";
			case game::MOD_GRENADE: return "MOD_GRENADE";
			case game::MOD_GRENADE_SPLASH: return "MOD_GRENADE_SPLASH";
			case game::MOD_PROJECTILE: return "MOD_PROJECTILE";
			case game::MOD_PROJECTILE_SPLASH: return "MOD_PROJECTILE_SPLASH";
			case game::MOD_MELEE: return "MOD_MELEE";
			case game::MOD_HEAD_SHOT: return "MOD_HEAD_SHOT";
			case game::MOD_CRUSH: return "MOD_CRUSH";
			case game::MOD_FALLING: return "MOD_FALLING";
			case game::MOD_SUICIDE: return "MOD_SUICIDE";
			case game::MOD_TRIGGER_HURT: return "MOD_TRIGGER_HURT";
			case game::MOD_EXPLOSIVE: return "MOD_EXPLOSIVE";
			case game::MOD_IMPACT: return "MOD_IMPACT";
			default: return "badMOD";
			}
		}

		void scr_player_killed_stub(game::gentity_s* self, const game::gentity_s* inflictor, game::gentity_s* attacker, int damage,
			int meansOfDeath, const unsigned int weapon, bool isAlternate, const float* vDir, const unsigned int hitLoc, int psTimeOffset, int deathAnimDuration)
		{
			const std::string _hitLoc = reinterpret_cast<const char**>(0x8AB828)[hitLoc];
			const auto _mod = convert_mod(meansOfDeath);

			const auto _weapon = get_weapon_name(weapon, isAlternate);

			for (const auto& callback : player_killed_callbacks)
			{
				const auto state = callback.lua_state();

				const auto _self = convert_entity(state, self);
				const auto _inflictor = convert_entity(state, inflictor);
				const auto _attacker = convert_entity(state, attacker);

				const auto _vDir = convert_vector(state, vDir);

				const auto result = callback(_self, _inflictor, _attacker, damage, _mod, _weapon, _vDir, _hitLoc, psTimeOffset, deathAnimDuration);

				scripting::lua::handle_error(result);

				if (result.valid() && result.get_type() == sol::type::number)
				{
					damage = result.get<int>();
				}
			}

			if (damage == 0)
			{
				return;
			}

			scr_player_killed_hook.invoke<void>(self, inflictor, attacker, damage, meansOfDeath, weapon, isAlternate, vDir, hitLoc, psTimeOffset, deathAnimDuration);
		}

		void scr_player_damage_stub(game::gentity_s* self, const game::gentity_s* inflictor, game::gentity_s* attacker, int damage, int dflags,
			int meansOfDeath, const unsigned int weapon, bool isAlternate, const float* vPoint, const float* vDir, const unsigned int hitLoc, int timeOffset)
		{
			const std::string _hitLoc = reinterpret_cast<const char**>(0x8AB828)[hitLoc];
			const auto _mod = convert_mod(meansOfDeath);

			const auto _weapon = get_weapon_name(weapon, isAlternate);

			for (const auto& callback : player_damage_callbacks)
			{
				const auto state = callback.lua_state();

				const auto _self = convert_entity(state, self);
				const auto _inflictor = convert_entity(state, inflictor);
				const auto _attacker = convert_entity(state, attacker);

				const auto _vPoint = convert_vector(state, vPoint);
				const auto _vDir = convert_vector(state, vDir);

				const auto result = callback(_self, _inflictor, _attacker, damage, dflags, _mod, _weapon, _vPoint, _vDir, _hitLoc);

				scripting::lua::handle_error(result);

				if (result.valid() && result.get_type() == sol::type::number)
				{
					damage = result.get<int>();
				}
			}

			if (damage == 0)
			{
				return;
			}

			scr_player_damage_hook.invoke<void>(self, inflictor, attacker, damage, dflags, meansOfDeath, weapon, isAlternate, vPoint, vDir, hitLoc, timeOffset);
		}

		void trigger_cmd_notifies(int clientNum, int key)
		{
			for (const auto& cmd : cmd_notifies[clientNum])
			{
				if (cmd.first == key)
				{
					const auto _player = scripting::call("getentbynum", {clientNum});

					if (_player.get_raw().type == game::SCRIPT_OBJECT)
					{
						const auto player = _player.as<scripting::entity>();
						scripting::notify(player, cmd.second, {});
					}
				}
			}
		}

		void client_command_stub(int clientNum)
		{
			char cmd[1024] = { 0 };

			game::SV_Cmd_ArgvBuffer(0, cmd, 1024);

			auto hidden = false;
			if (cmd == "n"s && cmd_notifies.find(clientNum) != cmd_notifies.end())
			{
				const auto key = atoi(game::ConcatArgs(1));
				trigger_cmd_notifies(clientNum, key);
			}
			else if (cmd == "say"s || cmd == "say_team"s)
			{
				std::string _cmd = cmd;
				std::string message = game::ConcatArgs(1);
				message.erase(0, 1);

				scheduler::once([hidden, _cmd, message, clientNum]()
				{
					const auto teamchat = _cmd == "say_team"s;
					const scripting::entity level{*game::levelEntityId};
					const auto _player = scripting::call("getEntByNum", {clientNum});

					if (_player.get_raw().type == game::SCRIPT_OBJECT)
					{
						const auto player = _player.as<scripting::entity>();

						scripting::notify(level, "say", {player, message, teamchat});
						scripting::notify(player, "say", {message, teamchat});
					}
				});

				for (const auto& callback : player_say_callbacks)
				{
					const auto _player = scripting::call("getEntByNum", {clientNum}).as<scripting::entity>();
					const auto result = callback(_player, message, _cmd == "say_team");

					scripting::lua::handle_error(result);

					if (result.valid() && result.get_type() == sol::type::boolean)
					{
						hidden = result.get<bool>();
					}
				}
			}

			if (!hidden)
			{
				return client_command_hook.invoke<void>(clientNum);
			}
		}

		unsigned int local_id_to_entity(unsigned int local_id)
		{
			const auto variable = game::scr_VarGlob->objectVariableValue[local_id];
			return variable.u.f.next;
		}

		scripting::script_value return_value{};

		bool execute_vm_hook(unsigned int pos)
		{
			if (vm_execute_hooks.find(pos) == vm_execute_hooks.end())
			{
				return false;
			}

			if (!hook_enabled && pos > function_count)
			{
				hook_enabled = true;
				return false;
			}

			return_value = {};

			const auto hook = vm_execute_hooks[pos];
			const auto state = hook.lua_state();

			const auto self_id = local_id_to_entity(game::scr_VmPub->function_frame->fs.localId);
			const auto self = scripting::entity(self_id);

			std::vector<sol::lua_value> args;

			const auto top = game::scr_function_stack->top;

			for (auto* value = top; value->type != game::SCRIPT_END; --value)
			{
				args.push_back(scripting::lua::convert(state, *value));
			}

			const auto result = hook(self, sol::as_args(args));
			scripting::lua::handle_error(result);

			const auto value = scripting::lua::convert({state, result});
			const auto type = value.get_raw().type;

			if (result.valid() && type && type != game::SCRIPT_END)
			{
				return_value = value;
			}

			return true;
		}

		void push_return_value()
		{
			const auto raw = return_value.get_raw();

			if (raw.type != game::SCRIPT_NONE)
			{
				++game::scr_function_stack->top;
				game::AddRefToValue(raw.type, raw.u);
				game::scr_function_stack->top->type = raw.type;
				game::scr_function_stack->top->u = raw.u;
				return_value = {};
			}
		}

		__declspec(naked) void vm_execute_return_stub()
		{
			__asm
			{
				pushad
				call push_return_value
				popad

				mov esi, 0x20B21FC
				sub [esi], eax

				mov esi, 0x20B4A60
				mov eax, [esi]

				push 0x56DBAA
				retn
			}
		}

		__declspec(naked) void vm_execute_return_stub2()
		{
			__asm
			{
				pushad
				call push_return_value
				popad

				push 0x56B720
				retn
			}
		}

		__declspec(naked) void vm_execute_stub()
		{
			__asm
			{
				pushad
				push esi
				call execute_vm_hook

				cmp al, 0
				jne void_call

				pop esi
				popad

				jmp loc_1
			loc_1:
				movzx eax, byte ptr[esi]
				inc esi

				mov [ebp - 0x18], eax
				mov [ebp - 0x8], esi

				push ecx

				mov ecx, 0x20B8E28
				mov [ecx], eax

				mov ecx, 0x20B4A5C
				mov[ecx], esi

				pop ecx

				cmp eax, 0x98

				push 0x56B740
				retn
			void_call:
				pop esi
				popad

				mov esi, empty_function_pos

				jmp loc_1
			}
		}
	}

	void clear_cmd_notifies(const scripting::entity& entity)
	{
		const auto clientNum = entity.call("getentitynumber", {}).as<int>();
		cmd_notifies[clientNum] = {};
	}

	void add_cmd_notify(int clientNum, const std::string& cmd, const std::string& notify)
	{
		const auto keynum = game::Key_GetBindingForCmd(cmd.data());

		if (!keynum)
		{
			return;
		}

		cmd_notifies[clientNum].push_back({keynum, notify});
	}

	void add_player_damage_callback(const sol::protected_function& callback)
	{
		player_damage_callbacks.push_back(callback);
	}

	void add_player_killed_callback(const sol::protected_function& callback)
	{
		player_killed_callbacks.push_back(callback);
	}

	void add_player_say_callback(const sol::protected_function& callback)
	{
		player_say_callbacks.push_back(callback);
	}

	void clear_callbacks()
	{
		function_count = 0;
		vm_execute_hooks.clear();

		cmd_notifies.clear();

		player_damage_callbacks.clear();
		player_killed_callbacks.clear();
		player_say_callbacks.clear();
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			client_command_hook.create(0x502CB0, client_command_stub);

			scr_player_damage_hook.create(0x527B90, scr_player_damage_stub);
			scr_player_killed_hook.create(0x527CF0, scr_player_killed_stub);

			utils::hook::jump(0x56B726, vm_execute_stub);
			utils::hook::jump(0x56DB9F, vm_execute_return_stub);
			utils::hook::jump(0x56B8AD, vm_execute_return_stub2);
		}
	};
}

REGISTER_COMPONENT(notifies::component)