// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <Psapi.h>
#include <thread>
#include <chrono>
#include "game/HksState.h"
#include "include/MinHook.h"
#include "include/Logger.h"
#include "game/ProcessData.h"
#include "game/AOBScan.h"

static void otherHooks();
#if _WIN64
#pragma comment(lib, "libMinHook-x64-v141-md.lib")
#else
#pragma comment(lib, "libMinHook-x86-v141-md.lib")
#endif

#if _DEBUG
#define DEBUG true
#else
#define DEBUG false
#endif

#if DEBUG
static inline void printAddresses() 
{
    Logger::debug("---");
    Logger::debug("Game: %p", getProcessBase());
    Logger::debug("---");
    Logger::debug("Addresses:");
    for (auto const& [name, address] : registeredAddresses) 
    {
        Logger::debug("%s: %p", name.c_str(), address);
    }
    Logger::debug("%s: %p", "newEnvFunc", newEnvFunc);
    Logger::debug("%s: %p", "newActFunc", newActFunc);
    Logger::debug("---");
}
#else
static inline void printAddresses() {};
#endif


static void initAddresses()
{
    GetText();
    ScanAndAssignAddresses();
    printAddresses();
}


bool createHook(const char* name, LPVOID pTarget, LPVOID pDetour, LPVOID* ppOriginal)
{
    int mhStatus = MH_CreateHook(pTarget, pDetour, ppOriginal);
    if (mhStatus != MH_OK)
    {
        Logger::log("MinHook CreateHook error creating hook \"%s\" (%d)", name, mhStatus);
        return false;
    }
    return true;
}
// also check for updates automatically with http coz i'm a creep, i'm a weirdo
void initHooks() 
{
    createHook("hksSetCGlobals", replacedHksSetCGlobals, &hksSetCGlobalsHookFunc, (void**)&hksSetCGlobals);

    Logger::log("Other hooks starting...");

    otherHooks();

    MH_EnableHook(MH_ALL_HOOKS);
}

void onAttach()
{
    if (GetConsoleWindow() == NULL) 
    {
        AllocConsole();
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
        Logger::log("Created Scripts-Data-Exposer-FS Console");

    }

    Logger::debug("Start onAttach");

    initBase();

    int mhStatus = MH_Initialize();
    if (mhStatus != MH_OK) 
    {
        Logger::log("MinHook Initialize error " + mhStatus);
        return;
    }

    initAddresses();

    initHooks();

    initialize_lua_state();
    //todo: remove hardcoded path and put in the ini file
    std::string path = "scriptExposerAssets/Start.lua";

    run_lua_file(path.c_str());

    Logger::debug("Finished onAttach");
}

void onDetach() 
{
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    cleanup_lua_state();
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::this_thread::sleep_for(std::chrono::seconds(10));
        onAttach();
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        onDetach();
        break;
    }
    return TRUE;
}

