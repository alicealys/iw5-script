#include <stdinc.hpp>
#include "context.hpp"
#include "error.hpp"
#include "value_conversion.hpp"

namespace scripting::lua
{
	event_handler::event_handler(sol::state& state)
		: state_(state)
	{
		auto event_listener_handle_type = state.new_usertype<event_listener_handle>("event_listener_handle");

		event_listener_handle_type["clear"] = [this](const event_listener_handle& handle)
		{
			this->remove(handle);
		};
	}

	void event_handler::dispatch(const event& event)
	{
		std::vector<sol::lua_value> arguments;

		for (const auto& argument : event.arguments)
		{
			arguments.emplace_back(convert(this->state_, argument));
		}

		this->dispatch_to_specific_listeners(event, arguments);
	}

	void event_handler::dispatch_to_specific_listeners(const event& event,
	                                                   const event_arguments& arguments)
	{
		for (auto listener : this->event_listeners_)
		{
			if (listener->event == event.name && listener->entity == event.entity)
			{
				if (listener->is_volatile)
				{
					this->event_listeners_.remove(listener);
				}

				handle_error(listener->callback(sol::as_args(arguments)));
			}
		}
	}

	event_listener_handle event_handler::add_event_listener(event_listener&& listener)
	{
		const uint64_t id = ++this->current_listener_id_;
		listener.id = id;
		this->event_listeners_.add(std::move(listener));
		return {id};
	}

	void event_handler::clear()
	{
		this->event_listeners_.clear();
	}

	void event_handler::remove(const event_listener_handle& handle)
	{
		for (const auto task : this->event_listeners_)
		{
			if (task->id == handle.id)
			{
				this->event_listeners_.remove(task);
				return;
			}
		}
	}
}
