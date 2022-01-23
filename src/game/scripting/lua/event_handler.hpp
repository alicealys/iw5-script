#pragma once
#include <utils/concurrent_list.hpp>

namespace scripting::lua
{
	using event_arguments = std::vector<sol::lua_value>;
	using event_callback = sol::protected_function;

	class event_listener_handle
	{
	public:
		unsigned long long id = 0;
	};

	class event_listener final : public event_listener_handle
	{
	public:
		std::string event = {};
		entity entity{};
		event_callback callback = {};
		bool is_volatile = false;
	};

	class event_handler final
	{
	public:
		event_handler(sol::state& state);

		event_handler(event_handler&&) noexcept = delete;
		event_handler& operator=(event_handler&&) noexcept = delete;

		event_handler(const scheduler&) = delete;
		event_handler& operator=(const event_handler&) = delete;

		void dispatch(const event& event);

		event_listener_handle add_event_listener(event_listener&& listener);

		void clear();

	private:
		sol::state& state_;
		std::atomic_int64_t current_listener_id_ = 0;

		utils::concurrent_list<event_listener> event_listeners_;

		void dispatch_to_specific_listeners(const event& event, const event_arguments& arguments);

		void remove(const event_listener_handle& handle);
	};
}
