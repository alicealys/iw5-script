#pragma once

#pragma warning(disable: 4244)
#pragma warning(disable: 26812)

#define DLL_EXPORT extern "C" __declspec(dllexport)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vector>
#include <cassert>
#include <mutex>
#include <string>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <regex>
#include <queue>
#include <unordered_set>
#include <filesystem>
#include <map>
#include <csetjmp>

using namespace std::literals;

#include <MinHook.h>

#include "utils/memory.hpp"
#include "utils/string.hpp"
#include "utils/hook.hpp"
#include "utils/concurrent_list.hpp"
#include "utils/io.hpp"

#include "game/structs.hpp"
#include "game/game.hpp"

#include "component/scheduler.hpp"
#include "component/scripting.hpp"
#include "component/notifies.hpp"

#define SELECT(mp, zm) (game::environment::t6mp() ? mp : zm)
