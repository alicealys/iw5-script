#include <stdinc.hpp>
#include "context.hpp"
#include "error.hpp"
#include "value_conversion.hpp"

#include "../execution.hpp"
#include "../functions.hpp"
#include "../array.hpp"

#include "../../../component/scripting.hpp"
#include "../../../component/notifies.hpp"
#include "../../../component/command.hpp"
#include "../../../component/scheduler.hpp"

#include <utils/string.hpp>

namespace scripting::lua
{
	namespace
	{
		std::unordered_map<std::string, script_value> pers_table;

		std::vector<std::string> load_game_constants()
		{
			std::vector<std::string> constants{};

			for (const auto& class_ : fields_table)
			{
				for (const auto& constant : class_.second)
				{
					constants.push_back(constant.first);
				}
			}

			return constants;
		}

		const std::vector<std::string>& get_game_constants()
		{
			static auto constants = load_game_constants();
			return constants;
		}

		std::string get_as_string(const sol::this_state s, sol::object o)
		{
			sol::state_view state(s);
			return state["tostring"](o);
		}

		struct http_request
		{
			sol::protected_function on_error;
			sol::protected_function on_progress;
			sol::protected_function on_load;
		};

		std::unordered_map<uint64_t, http_request> http_requests;

		void setup_entity_type(sol::state& state, event_handler& handler, scheduler& scheduler)
		{
			state["level"] = entity{*game::levelEntityId};

			state["io"]["fileexists"] = utils::io::file_exists;
			state["io"]["writefile"] = utils::io::write_file;
			state["io"]["filesize"] = utils::io::file_size;
			state["io"]["createdirectory"] = utils::io::create_directory;
			state["io"]["directoryexists"] = utils::io::directory_exists;
			state["io"]["directoryisempty"] = utils::io::directory_is_empty;
			state["io"]["listfiles"] = utils::io::list_files;
			state["io"]["copyfolder"] = utils::io::copy_folder;
			state["io"]["readfile"] = [](const std::string& path)
			{
				return utils::io::read_file(path);
			};

			auto vector_type = state.new_usertype<vector>("vector", sol::constructors<vector(float, float, float)>());
			vector_type["x"] = sol::property(&vector::get_x, &vector::set_x);
			vector_type["y"] = sol::property(&vector::get_y, &vector::set_y);
			vector_type["z"] = sol::property(&vector::get_z, &vector::set_z);

			vector_type["r"] = sol::property(&vector::get_x, &vector::set_x);
			vector_type["g"] = sol::property(&vector::get_y, &vector::set_y);
			vector_type["b"] = sol::property(&vector::get_z, &vector::set_z);

			auto array_type = state.new_usertype<array>("array", sol::constructors<array()>());

			array_type["erase"] = [](const array& array, const sol::this_state s,
				const sol::lua_value& key)
			{
				if (key.is<int>())
				{
					const auto index = key.as<int>() - 1;
					array.erase(index);
				}
				else if (key.is<std::string>())
				{
					const auto _key = key.as<std::string>();
					array.erase(_key);
				}
			};

			array_type["push"] = [](const array& array, const sol::this_state s,
				const sol::lua_value& value)
			{
				const auto _value = convert(value);
				array.push(_value);
			};

			array_type["pop"] = [](const array& array, const sol::this_state s)
			{
				return convert(s, array.pop());
			};

			array_type["get"] = [](const array& array, const sol::this_state s,
				const sol::lua_value& key)
			{
				if (key.is<int>())
				{
					const auto index = key.as<int>() - 1;
					return convert(s, array.get(index));
				}
				else if (key.is<std::string>())
				{
					const auto _key = key.as<std::string>();
					return convert(s, array.get(_key));
				}

				return sol::lua_value{s, sol::lua_nil};
			};

			array_type["set"] = [](const array& array, const sol::this_state s,
				const sol::lua_value& key, const sol::lua_value& value)
			{
				const auto _value = convert(value);
				const auto nil = _value.get_raw().type == 0;

				if (key.is<int>())
				{
					const auto index = key.as<int>() - 1;
					nil ? array.erase(index) : array.set(index, _value);
				}
				else if (key.is<std::string>())
				{
					const auto _key = key.as<std::string>();
					nil ? array.erase(_key) : array.set(_key, _value);
				}
			};

			array_type["size"] = [](const array& array, const sol::this_state s)
			{
				return array.size();
			};

			array_type[sol::meta_function::length] = [](const array& array, const sol::this_state s)
			{
				return array.size();
			};

			array_type[sol::meta_function::index] = [](const array& array, const sol::this_state s,
				const sol::lua_value& key)
			{
				if (key.is<int>())
				{
					const auto index = key.as<int>() - 1;
					return convert(s, array.get(index));
				}
				else if (key.is<std::string>())
				{
					const auto _key = key.as<std::string>();
					return convert(s, array.get(_key));
				}

				return sol::lua_value{s, sol::lua_nil};
			};

			array_type[sol::meta_function::new_index] = [](const array& array, const sol::this_state s,
				const sol::lua_value& key, const sol::lua_value& value)
			{
				const auto _value = convert(value);
				const auto nil = _value.get_raw().type == 0;

				if (key.is<int>())
				{
					const auto index = key.as<int>() - 1;
					nil ? array.erase(index) : array.set(index, _value);
				}
				else if (key.is<std::string>())
				{
					const auto _key = key.as<std::string>();
					nil ? array.erase(_key) : array.set(_key, _value);
				}
			};

			array_type["getkeys"] = [](const array& array, const sol::this_state s)
			{
				std::vector<sol::lua_value> keys;

				const auto keys_ = array.get_keys();
				for (const auto& key : keys_)
				{
					keys.push_back(convert(s, key));
				}
				
				return keys;
			};

			auto entity_type = state.new_usertype<entity>("entity");

			for (const auto& func : method_map)
			{
				const auto name = utils::string::to_lower(func.first);
				entity_type[name.data()] = [name](const entity& entity, const sol::this_state s, sol::variadic_args va)
				{
					std::vector<script_value> arguments{};

					for (auto arg : va)
					{
						arguments.push_back(convert({s, arg}));
					}

					return convert(s, entity.call(name, arguments));
				};
			}

			entity_type["notifyonplayercommand"] = [](const entity& entity, const sol::this_state s, const std::string& notify, const std::string& cmd)
			{
				const auto entnum = entity.call("getentitynumber").as<int>();
				notifies::add_cmd_notify(entnum, cmd, notify);
			};

			for (const auto& constant : get_game_constants())
			{
				entity_type[constant] = sol::property(
					[constant](const entity& entity, const sol::this_state s)
					{
						return convert(s, entity.get(constant));
					},
					[constant](const entity& entity, const sol::this_state s, const sol::lua_value& value)
					{
						entity.set(constant, convert({s, value}));
					});
			}

			entity_type["set"] = [](const entity& entity, const std::string& field,
			                        const sol::lua_value& value)
			{
				entity.set(field, convert(value));
			};

			entity_type["get"] = [](const entity& entity, const sol::this_state s, const std::string& field)
			{
				return convert(s, entity.get(field));
			};

			entity_type["notify"] = [](const entity& entity, const sol::this_state s, const std::string& event, 
									   sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				notify(entity, event, arguments);
			};

			entity_type["onnotify"] = [&handler](const entity& entity, const sol::this_state s, 
				const std::string& event, const event_callback& callback)
			{
				event_listener listener{};
				listener.callback = {s, callback};
				listener.entity = entity;
				listener.event = event;
				listener.is_volatile = false;

				return handler.add_event_listener(std::move(listener));
			};

			entity_type["onnotifyonce"] = [&handler](const entity& entity, const sol::this_state s,
				const std::string& event, const event_callback& callback)
			{
				event_listener listener{};
				listener.callback = {s, callback};
				listener.entity = entity;
				listener.event = event;
				listener.is_volatile = true;

				return handler.add_event_listener(std::move(listener));
			};

			entity_type["call"] = [](const entity& entity, const sol::this_state s, const std::string& function,
			                         sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				return convert(s, entity.call(function, arguments));
			};

			entity_type["sendservercommand"] = [](const entity& entity, const sol::this_state s, const std::string& command)
			{
				const auto entref = entity.get_entity_reference();

				if (entref.classnum != 0 || entref.entnum > 17)
				{
					return;
				}

				::game::SV_GameSendServerCommand(entref.entnum, 0, command.data());
			};

			entity_type["tell"] = [](const entity& entity, const sol::this_state s, const std::string& message)
			{
				const auto entref = entity.get_entity_reference();

				if (entref.classnum != 0 || entref.entnum > 17)
				{
					return;
				}

				::game::SV_GameSendServerCommand(entref.entnum, 0, utils::string::va("%c \"%s\"",
					84, message.data()));
			};

			entity_type[sol::meta_function::new_index] = [](const entity& entity, const std::string& field,
				const sol::lua_value& value)
			{
				entity.set(field, convert(value));
			};

			entity_type[sol::meta_function::index] = [](const entity& entity, const sol::this_state s, const std::string& field)
			{
				return convert(s, entity.get(field));
			};

			entity_type["getstruct"] = [](const entity& entity, const sol::this_state s)
			{
				const auto id = entity.get_entity_id();
				return entity_to_struct(s, id);
			};

			entity_type["struct"] = sol::property([](const entity& entity, const sol::this_state s)
			{
				const auto id = entity.get_entity_id();
				return entity_to_struct(s, id);
			});

			entity_type["scriptcall"] = [](const entity& entity, const sol::this_state s, const std::string& filename,
				const std::string function, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				notifies::hook_enabled = false;
				const auto result = convert(s, call_script_function(entity, filename, function, arguments));
				notifies::hook_enabled = true;

				return result;
			};

			entity_type["flags"] = sol::property(
				[](const entity& entity, const sol::this_state s)
				{
					const auto entref = entity.get_entity_reference();
					if (entref.classnum != 0 || entref.entnum > 17)
					{
						return 0;
					}

					return game::g_entities[entref.entnum].flags;
				},
				[](const entity& entity, const sol::this_state s, const int value)
				{
					const auto entref = entity.get_entity_reference();
					if (entref.classnum != 0 || entref.entnum > 17)
					{
						return;
					}

					game::g_entities[entref.entnum].flags = value;
				}
			);

			entity_type["clientflags"] = sol::property(
				[](const entity& entity, const sol::this_state s)
				{
					const auto entref = entity.get_entity_reference();
					if (entref.classnum != 0 || entref.entnum > 17)
					{
						return 0;
					}

					return game::g_entities[entref.entnum].client->flags;
				},
				[](const entity& entity, const sol::this_state s, const int value)
				{
					const auto entref = entity.get_entity_reference();
					if (entref.classnum != 0 || entref.entnum > 17)
					{
						return;
					}

					game::g_entities[entref.entnum].client->flags = value;
				}
			);

			entity_type["address"] = sol::property(
				[](const entity& entity, const sol::this_state s)
				{
					const auto entref = entity.get_entity_reference();

					if (entref.classnum != 0 || entref.entnum > 17)
					{
						return sol::lua_value{s, sol::lua_nil};
					}

					const auto address = game::svs_clients[entref.entnum].remote.ip;
					const std::string address_str = utils::string::va("%i.%i.%i.%i", 
						address[0],
						address[1], 
						address[2], 
						address[3]
					);

					return sol::lua_value{address_str};
				}
			);

			entity_type["updatetext"] = [](const entity& entity, const sol::this_state s,
				const std::string& text)
			{
				const auto entref = entity.get_entity_reference();
				if (entref.classnum != 1)
				{
					return;
				}

				auto elem = &game::g_hudelems[entref.entnum].elem;
				if (!elem->text)
				{
					entity.call("settext", {text});
					return;
				}

				game::SV_SetConfigstring(elem->text + 469, text.data());
			};

			struct game
			{
			};
			auto game_type = state.new_usertype<game>("game_");
			state["game"] = game();

			for (const auto& func : function_map)
			{
				const auto name = utils::string::to_lower(func.first);
				game_type[name] = [name](const game&, const sol::this_state s, sol::variadic_args va)
				{
					std::vector<script_value> arguments{};

					for (auto arg : va)
					{
						arguments.push_back(convert({s, arg}));
					}

					return convert(s, call(name, arguments));
				};
			}

			game_type["call"] = [](const game&, const sol::this_state s, const std::string& function,
			                       sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				return convert(s, call(function, arguments));
			};

			game_type["ontimeout"] = [&scheduler](const game&, const sol::this_state s, 
				const sol::protected_function& callback, const long long milliseconds)
			{
				return scheduler.add({s, callback}, milliseconds, true);
			};

			game_type["oninterval"] = [&scheduler](const game&, const sol::this_state s, 
				const sol::protected_function& callback, const long long milliseconds)
			{
				return scheduler.add({s, callback}, milliseconds, false);
			};

			game_type["executecommand"] = [](const game&, const std::string& command)
			{
				::game::Cbuf_AddText(0, command.data());
			};

			game_type["sendservercommand"] = [](const game&, const int client, const std::string& command)
			{
				::game::SV_GameSendServerCommand(client, 0, command.data());
			};

			game_type["say"] = [](const game&, const sol::this_state s, const std::string& message)
			{
				::game::SV_GameSendServerCommand(-1, 0, utils::string::va("%c \"%s\"",
					84, message.data()));
			};

			game_type["onplayerdamage"] = [](const game&, const sol::this_state s, 
				const sol::protected_function& callback)
			{
				notifies::add_player_damage_callback({s, callback});
			};

			game_type["onplayerkilled"] = [](const game&, const sol::this_state s, 
				const sol::protected_function& callback)
			{
				notifies::add_player_killed_callback({s, callback});
			};

			game_type["onplayersay"] = [](const game&, const sol::this_state s, 
				const sol::protected_function& callback)
			{
				notifies::add_player_say_callback({s, callback});
			};

			game_type["scriptcall"] = [](const game&, const sol::this_state s, const std::string& filename,
				const std::string function, sol::variadic_args va)
			{
				std::vector<script_value> arguments{};

				for (auto arg : va)
				{
					arguments.push_back(convert({s, arg}));
				}

				const auto level = entity{*::game::levelEntityId};

				notifies::hook_enabled = false;
				const auto result = convert(s, call_script_function(level, filename, function, arguments));
				notifies::hook_enabled = true;

				return result;
			};

			game_type["detour"] = [](const game&, const sol::this_state s, const std::string& filename,
				const std::string function_name, const sol::protected_function& function)
			{
				const auto pos = get_function_pos(filename, function_name);
				notifies::vm_execute_hooks[pos] = function;

				auto detour = sol::table::create(function.lua_state());

				detour["disable"] = [pos]()
				{
					notifies::vm_execute_hooks.erase(pos);
				};

				detour["enable"] = [pos, function]()
				{
					notifies::vm_execute_hooks[pos] = function;
				};

				detour["invoke"] = [filename, function_name](const entity& entity, const sol::this_state s, sol::variadic_args va)
				{
					std::vector<script_value> arguments{};

					for (auto arg : va)
					{
						arguments.push_back(convert({s, arg}));
					}

					notifies::hook_enabled = false;
					const auto result = convert(s, call_script_function(entity, filename, function_name, arguments));
					notifies::hook_enabled = true;

					return result;
				};

				return detour;
			};

			game_type["getfunctions"] = [entity_type](const game&, const sol::this_state s, const std::string& filename)
			{
				if (scripting::script_function_table.find(filename) == scripting::script_function_table.end())
				{
					throw std::runtime_error("File '" + filename + "' not found");
				}

				auto functions = sol::table::create(s.lua_state());

				for (const auto& function : scripting::script_function_table[filename])
				{
					functions[function.first] = [filename, function](const entity& entity, const sol::this_state s, sol::variadic_args va)
					{
						std::vector<script_value> arguments{};

						for (auto arg : va)
						{
							arguments.push_back(convert({s, arg}));
						}

						notifies::hook_enabled = false;
						const auto result = convert(s, call_script_function(entity, filename, function.first, arguments));
						notifies::hook_enabled = true;

						return result;
					};
				}

				return functions;
			};

			game_type["getgamevar"] = [](const sol::this_state s)
			{
				const auto id = *::game::gameEntityId;
				const auto value = ::game::scr_VarGlob->childVariableValue[id];
				return array(value.u.u.uintValue);
			};

			game_type["addcommand"] = [](const game&, const sol::this_state,
				const std::string& name, const sol::protected_function& callback)
			{
				command::add_script_command(name, [callback](const command::params& params)
				{
					auto args = sol::table::create(callback.lua_state());
					for (auto i = 0; i < params.size(); i++)
					{
						args[i + 1] = params[i];
					}

					callback(args);
				});
			};

			game_type["include"] = [](const game&, const sol::this_state s,
				const std::string& filename)
			{
				sol::state_view state = s;

				if (scripting::script_function_table.find(filename) == scripting::script_function_table.end())
				{
					throw std::runtime_error("File '" + filename + "' not found");
				}

				for (const auto& function : scripting::script_function_table[filename])
				{
					const auto name = utils::string::to_lower(function.first);

					state["game_"][name] = [filename, function](const game&, const sol::this_state s,
						sol::variadic_args va)
					{
						std::vector<script_value> arguments{};

						for (auto arg : va)
						{
							arguments.push_back(convert({s, arg}));
						}

						notifies::hook_enabled = false;
						const auto result = convert(s, call_script_function(*::game::levelEntityId, filename, function.first, arguments));
						notifies::hook_enabled = true;
					};

					state["entity"][name] = [filename, function](const entity& entity, const sol::this_state s,
						sol::variadic_args va)
					{
						std::vector<script_value> arguments{};

						for (auto arg : va)
						{
							arguments.push_back(convert({s, arg}));
						}

						notifies::hook_enabled = false;
						const auto result = convert(s, call_script_function(entity, filename, function.first, arguments));
						notifies::hook_enabled = true;
					};
				}
			};

			game_type["getvarusage"] = [](const game&)
			{
				auto count = 0;
				for (auto i = 0; i < 36864; i++)
				{
					const auto value = ::game::scr_VarGlob->objectVariableValue[i];
					count += value.w.type != 24;
				}
				return count;
			};

			game_type["getchildvarusage"] = [](const game&)
			{
				auto count = 0;
				for (auto i = 0; i < 102400; i++)
				{
					const auto value = ::game::scr_VarGlob->childVariableValue[i];
					count += value.type != 24;
				}
				return count;
			};

			static uint64_t task_id = 0;

			state["http"] = sol::table::create(state.lua_state());

			state["http"]["get"] = [](const sol::this_state, const std::string& url,
				const sol::protected_function& callback, sol::variadic_args va)
			{
				bool async = false;

				if (va.size() >= 1 && va[0].get_type() == sol::type::boolean)
				{
					async = va[0].as<bool>();
				}

				const auto cur_task_id = task_id++;
				auto request_callbacks = &http_requests[cur_task_id];
				request_callbacks->on_load = callback;

				::scheduler::once([url, cur_task_id]()
				{
					if (http_requests.find(cur_task_id) == http_requests.end())
					{
						return;
					}

					const auto data = utils::http::get_data(url);
					::scheduler::once([data, cur_task_id]()
					{
						if (http_requests.find(cur_task_id) == http_requests.end())
						{
							return;
						}

						const auto& request_callbacks_ = http_requests[cur_task_id];
						const auto has_value = data.has_value();
						handle_error(request_callbacks_.on_load(has_value ? data.value().buffer : "", has_value));
						http_requests.erase(cur_task_id);
					}, ::scheduler::pipeline::server);
				}, ::scheduler::pipeline::async);
			};

			state["http"]["request"] = [](const sol::this_state s, const std::string& url, sol::variadic_args va)
			{
				auto request = sol::table::create(s.lua_state());

				std::string buffer{};
				std::string fields_string{};
				std::unordered_map<std::string, std::string> headers_map;

				if (va.size() >= 1 && va[0].get_type() == sol::type::table)
				{
					const auto options = va[0].as<sol::table>();

					const auto fields = options["parameters"];
					const auto body = options["body"];
					const auto headers = options["headers"];

					if (fields.get_type() == sol::type::table)
					{
						const auto _fields = fields.get<sol::table>();

						for (const auto& field : _fields)
						{
							const auto key = field.first.as<std::string>();
							const auto value = get_as_string(s, field.second);

							fields_string += key + "=" + value + "&";
						}
					}

					if (body.get_type() == sol::type::string)
					{
						fields_string = body.get<std::string>();
					}

					if (headers.get_type() == sol::type::table)
					{
						const auto _headers = headers.get<sol::table>();

						for (const auto& header : _headers)
						{
							const auto key = header.first.as<std::string>();
							const auto value = get_as_string(s, header.second);

							headers_map[key] = value;
						}
					}
				}

				request["onerror"] = []() {};
				request["onprogress"] = []() {};
				request["onload"] = []() {};

				request["send"] = [url, fields_string, request, headers_map]()
				{
					const auto cur_task_id = task_id++;
					auto request_callbacks = &http_requests[cur_task_id];
					request_callbacks->on_error = request["onerror"];
					request_callbacks->on_progress = request["onprogress"];
					request_callbacks->on_load = request["onload"];

					::scheduler::once([url, fields_string, cur_task_id, headers_map]()
					{
						if (http_requests.find(cur_task_id) == http_requests.end())
						{
							return;
						}

						const auto result = utils::http::get_data(url, fields_string, headers_map, [cur_task_id](size_t value)
						{
							::scheduler::once([cur_task_id, value]()
							{
								if (http_requests.find(cur_task_id) == http_requests.end())
								{
									return;
								}

								const auto& request_callbacks_ = http_requests[cur_task_id];
								handle_error(request_callbacks_.on_progress(value));
							}, ::scheduler::pipeline::server);
						});

						::scheduler::once([cur_task_id, result]()
						{
							if (http_requests.find(cur_task_id) == http_requests.end())
							{
								return;
							}

							const auto& request_callbacks_ = http_requests[cur_task_id];

							if (!result.has_value())
							{
								request_callbacks_.on_error("Unknown", -1);
								return;
							}

							const auto& http_result = result.value();

							if (http_result.code == CURLE_OK)
							{
								handle_error(request_callbacks_.on_load(http_result.buffer));
							}
							else
							{
								handle_error(request_callbacks_.on_error(curl_easy_strerror(http_result.code), http_result.code));
							}

							http_requests.erase(cur_task_id);
						}, ::scheduler::pipeline::server);
					}, ::scheduler::pipeline::async);
				};

				return request;
			};

			state["pers"] = sol::table::create(state.lua_state());

			auto pers_metatable = sol::table::create(state.lua_state());

			pers_metatable[sol::meta_function::new_index] = [](const sol::table t, const sol::this_state s, 
				const std::string& key, const sol::lua_value& value)
			{
				pers_table[key] = convert({s, value});
			};

			pers_metatable[sol::meta_function::index] = [](const sol::table t, const sol::this_state s, 
				const std::string& key)
			{
				if (pers_table.find(key) == pers_table.end())
				{
					return sol::lua_value{s, sol::lua_nil};
				}

				return convert(s, pers_table[key]);
			};

			state["pers"][sol::metatable_key] = pers_metatable;
		}
	}

	context::context(std::string folder)
		: folder_(std::move(folder))
		  , scheduler_(state_)
		  , event_handler_(state_)

	{
		this->state_.open_libraries(sol::lib::base,
		                            sol::lib::package,
		                            sol::lib::io,
		                            sol::lib::string,
		                            sol::lib::os,
		                            sol::lib::math,
		                            sol::lib::table);

		this->state_["include"] = [this](const std::string& file)
		{
			this->load_script(file);
		};

		sol::function old_require = this->state_["require"];
		auto base_path = utils::string::replace(this->folder_, "/", ".") + ".";
		this->state_["require"] = [base_path, old_require](const std::string& path)
		{
			return old_require(base_path + path);
		};

		this->state_["scriptdir"] = [this]()
		{
			return this->folder_;
		};

		this->state_["print"] = [](const sol::this_state s, sol::variadic_args va)
		{
			sol::state_view view = s;
			for (const auto& arg : va)
			{
				const auto string = view["tostring"](arg).get<std::string>();
				printf("%s\t", string.data());
			}

			printf("\n");
		};

		setup_entity_type(this->state_, this->event_handler_, this->scheduler_);

		printf("Loading script '%s'\n", this->folder_.data());
		this->load_script("__init__");
	}

	context::~context()
	{
		http_requests.clear();
		this->state_.collect_garbage();
		this->scheduler_.clear();
		this->event_handler_.clear();
		this->state_ = {};
	}

	void context::run_frame()
	{
		this->scheduler_.run_frame();
		this->state_.collect_garbage();
	}

	void context::notify(const event& e)
	{
		this->event_handler_.dispatch(e);
	}

	void context::load_script(const std::string& script)
	{
		if (!this->loaded_scripts_.emplace(script).second)
		{
			return;
		}

		const auto file = (std::filesystem::path{this->folder_} / (script + ".lua")).generic_string();
		handle_error(this->state_.safe_script_file(file, &sol::script_pass_on_error));
	}
}
