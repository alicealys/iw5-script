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
	namespace
	{
		utils::hook::detour client_command_hook;

		utils::hook::detour scr_player_killed_hook;
		utils::hook::detour scr_player_damage_hook;

		std::vector<sol::protected_function> player_killed_callbacks;
		std::vector<sol::protected_function> player_damage_callbacks;

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

	void add_player_damage_callback(const sol::protected_function& callback)
	{
		player_damage_callbacks.push_back(callback);
	}

	void add_player_killed_callback(const sol::protected_function& callback)
	{
		player_killed_callbacks.push_back(callback);
	}

	void clear_callbacks()
	{
		player_damage_callbacks.clear();
		player_killed_callbacks.clear();
	}

	class component final : public component_interface
	{
	public:
		void post_unpack() override
		{
			client_command_hook.create(0x502CB0, client_command_stub);

			scr_player_damage_hook.create(0x527B90, scr_player_damage_stub);
			scr_player_killed_hook.create(0x527CF0, scr_player_killed_stub);
		}
	};
}

REGISTER_COMPONENT(notifies::component)