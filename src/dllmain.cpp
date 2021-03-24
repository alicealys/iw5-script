#include <stdinc.hpp>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        scheduler::init();
        scripting::init();
        notifies::init();
    }

    return TRUE;
}