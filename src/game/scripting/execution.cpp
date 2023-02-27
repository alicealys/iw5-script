#include <stdinc.hpp>
#include "execution.hpp"
#include "safe_execution.hpp"
#include "stack_isolation.hpp"
#include "event.hpp"
#include "lua/engine.hpp"

#include "component/scripting.hpp"
#include "component/scheduler.hpp"

namespace scripting
{
	namespace
	{
		game::VariableValue* allocate_argument()
		{
			game::VariableValue* value_ptr = ++game::scr_VmPub->top;
			++game::scr_VmPub->inparamcount;
			return value_ptr;
		}

		script_value get_return_value()
		{
			if (game::scr_VmPub->inparamcount == 0)
			{
				return {};
			}

			game::Scr_ClearOutParams();
			game::scr_VmPub->outparamcount = game::scr_VmPub->inparamcount;
			game::scr_VmPub->inparamcount = 0;

			return script_value(game::scr_VmPub->top[1 - game::scr_VmPub->outparamcount]);
		}

		int get_field_id(const int classnum, const std::string& field)
		{
			if (scripting::fields_table[classnum].find(field) != scripting::fields_table[classnum].end())
			{
				return scripting::fields_table[classnum][field];
			}

			return -1;
		}

		void scr_notify_id(int id, unsigned int stringValue, unsigned int paramcount)
		{
			if (game::scr_VmPub->outparamcount)
			{
				game::Scr_ClearOutParams();
			}

			auto v6 = game::scr_VmPub->top;
			auto v7 = game::scr_VmPub->inparamcount - paramcount;
			auto v8 = &game::scr_VmPub->top[-1 * static_cast<int>(paramcount)];

			if (id)
			{
				const auto v9 = v8->type;
				v8->type = game::scriptType_e::SCRIPT_END;

				game::scr_VmPub->inparamcount = 0;
				game::VM_Notify(id, stringValue, game::scr_VmPub->top);

				v8->type = v9;
				v6 = game::scr_VmPub->top;
			}

			for (; v6 != v8; game::scr_VmPub->top = v6)
			{
				game::RemoveRefToValue(v6->type, v6->u);
				v6 = game::scr_VmPub->top - 1;
			}

			game::scr_VmPub->inparamcount = v7;
		}

		bool is_entity_variable(const game::scr_entref_t& entref, const unsigned int id)
		{
			const auto type = game::scr_VarGlob->objectVariableValue[id].w.type;
			if (entref.classnum == 0)
			{
				return type == game::SCRIPT_ENTITY;
			}
			else if (entref.classnum > 0)
			{
				return true;
			}

			return false;
		}
	}

	void push_value(const script_value& value)
	{
		auto* value_ptr = allocate_argument();
		*value_ptr = value.get_raw();

		game::AddRefToValue(value_ptr->type, value_ptr->u);
	}

	void notify(const entity& entity, const std::string& event, const std::vector<script_value>& arguments)
	{
		stack_isolation _;
		for (auto i = arguments.rbegin(); i != arguments.rend(); ++i)
		{
			push_value(*i);
		}

		const auto event_id = game::SL_GetString(event.data(), 0);
		scr_notify_id(entity.get_entity_id(), event_id, game::scr_VmPub->inparamcount);
	}

	script_value call_function(const std::string& name, const entity& entity,
		const std::vector<script_value>& arguments)
	{
		const auto entref = entity.get_entity_reference();
		const auto ent_id = entity.get_entity_id();

		if (!is_entity_variable(entref, ent_id))
		{
			return {};
		}

		const auto is_method_call = *reinterpret_cast<const int*>(&entref) != -1;
		const auto function = find_function(name, !is_method_call);
		if (!function)
		{
			throw std::runtime_error("Unknown function '" + name + "'");
		}

		stack_isolation _;

		for (auto i = arguments.rbegin(); i != arguments.rend(); ++i)
		{
			push_value(*i);
		}

		game::scr_VmPub->outparamcount = game::scr_VmPub->inparamcount;
		game::scr_VmPub->inparamcount = 0;

		if (!safe_execution::call(function, entref))
		{
			throw std::runtime_error("Error executing function '" + name + "'");
		}

		return get_return_value();
	}

	script_value call_function(const std::string& name, const std::vector<script_value>& arguments)
	{
		return call_function(name, entity(), arguments);
	}

	template <>
	script_value call(const std::string& name, const std::vector<script_value>& arguments)
	{
		return call_function(name, arguments);
	}

	script_value exec_ent_thread(const entity& entity, unsigned int pos, const std::vector<script_value>& arguments)
	{
		const auto id = entity.get_entity_id();

		stack_isolation _;
		for (auto i = arguments.rbegin(); i != arguments.rend(); ++i)
		{
			scripting::push_value(*i);
		}

		game::AddRefToObject(id);

		const auto local_id = game::AllocThread(id);
		const auto result = game::VM_Execute(local_id, pos, arguments.size());
		game::RemoveRefToObject(result);

		return get_return_value();
	}

	unsigned int get_function_pos(const std::string& filename, const std::string& function)
	{
		const auto filename_lower = utils::string::to_lower(filename);
		const auto function_lower = utils::string::to_lower(function);

		if (!scripting::script_function_table.contains(filename_lower))
		{
			throw std::runtime_error("File '" + filename + "' not found");
		};

		const auto& functions = scripting::script_function_table[filename_lower];
		const auto function_iter = functions.find(function_lower);
		if (function_iter == functions.end())
		{
			throw std::runtime_error("Function '" + function + "' in file '" + filename + "' not found");
		}

		return function_iter->second;
	}

	script_value call_script_function(const entity& entity, const std::string& filename,
		const std::string& function, const std::vector<script_value>& arguments)
	{
		const auto pos = get_function_pos(filename, function);
		return exec_ent_thread(entity, pos, arguments);
	}

	void set_entity_field(const entity& entity, const std::string& field, const script_value& value)
	{
		const auto entref = entity.get_entity_reference();
		const int id = get_field_id(entref.classnum, field);
		const auto ent_id = entity.get_entity_id();

		if (id != -1 && is_entity_variable(entref, ent_id))
		{
			stack_isolation _;
			push_value(value);

			game::scr_VmPub->outparamcount = game::scr_VmPub->inparamcount;
			game::scr_VmPub->inparamcount = 0;

			if (!safe_execution::set_entity_field(entref, id))
			{
				throw std::runtime_error("Failed to set value for field '" + field + "'");
			}
		}
		else
		{
			set_object_variable(ent_id, field, value);
		}
	}

	script_value get_entity_field(const entity& entity, const std::string& field)
	{
		const auto entref = entity.get_entity_reference();
		const auto id = get_field_id(entref.classnum, field);
		const auto ent_id = entity.get_entity_id();

		if (id != -1 && is_entity_variable(entref, ent_id))
		{
			stack_isolation _;

			game::VariableValue value{};
			if (!safe_execution::get_entity_field(entref, id, &value))
			{
				throw std::runtime_error("Failed to get value for field '" + field + "'");
			}

			const auto __ = gsl::finally([value]()
			{
				game::RemoveRefToValue(value.type, value.u);
			});

			return value;
		}

		return get_object_variable(ent_id, field);
	}

	unsigned int make_array()
	{
		unsigned int index = 0;
		const auto variable = game::AllocVariable(&index);
		variable->w.type = game::SCRIPT_ARRAY;
		variable->u.f.prev = 0;
		variable->u.f.next = 0;

		return index;
	}

	void set_object_variable(const unsigned int parent_id, const unsigned int id, const script_value& value)
	{
		const auto offset = 0xC800 * (parent_id & 1);
		const auto variable_id = game::GetVariable(parent_id, id);
		const auto variable = &game::scr_VarGlob->childVariableValue[variable_id + offset];
		const auto& raw_value = value.get_raw();

		game::AddRefToValue(raw_value.type, raw_value.u);
		game::RemoveRefToValue(variable->type, variable->u.u);

		variable->type = static_cast<char>(raw_value.type);
		variable->u.u = raw_value.u;
	}

	void set_object_variable(const unsigned int parent_id, const std::string& name, const script_value& value)
	{
		const auto id = scripting::find_token_id(name);
		set_object_variable(parent_id, id, value);
	}

	script_value get_object_variable(const unsigned int parent_id, const unsigned int id)
	{
		const auto offset = 0xC800 * (parent_id & 1);
		const auto variable_id = game::FindVariable(parent_id, id);
		if (!variable_id)
		{
			return {};
		}

		const auto variable = &game::scr_VarGlob->childVariableValue[variable_id + offset];
		game::VariableValue value{};
		value.type = static_cast<game::scriptType_e>(variable->type);
		value.u = variable->u.u;

		return value;
	}

	script_value get_object_variable(const unsigned int parent_id, const std::string& name)
	{
		const auto id = scripting::find_token_id(name);
		return get_object_variable(parent_id, id);
	}
}