#pragma once
#include "../include/mem/mem.h"
#include "../include/mem/pattern.h"
#include "../include/mem/simd_scanner.h"
#include "../include/Logger.h"
#include "ProcessData.h"
#include <iomanip>
#include <sstream>
#include <map>

static void* text;
static size_t text_size;

static std::map<const char*, void*> registeredAddresses;

//Regular AOB scan
inline void* AOBScanAddress(const char* AOBString, const void* region = text, const size_t region_size = text_size) 
{
    const auto pattern = mem::pattern(AOBString);
    const auto mregion = mem::region(region, region_size);
    char* result = mem::simd_scanner(pattern).scan(mregion).any();

    if (!result) 
    {
        Logger::log("AOB string \"%s\" not found.", AOBString);
    }

    return reinterpret_cast<void*>(result);
}


//Regular AOB scan
inline void* AOBScanAddress(const unsigned char* AOBString, const char* AOBMask, const void* region = text, const size_t region_size = text_size)
{
    const auto pattern = mem::pattern(AOBString, AOBMask);
    const auto mregion = mem::region(region, region_size);
    char* result = mem::simd_scanner(pattern).scan(mregion).any();

    if (!result)
    {
        size_t AOBStrLen = std::strlen(AOBMask);
        std::stringstream str;
        str << "AOB string ";
        for (size_t i = 0; i < AOBStrLen; i++)
        {
            str << std::hex << static_cast<int>(AOBString[i]) << " ";
        }
        str << "not found.";
        Logger::log(str.str());
    }

    return reinterpret_cast<void*>(result);
}

//AOB scan with offset (e.g for when the AOB is inside the function)
inline void* AOBScanCode(const char* AOBString, const int Offset = 0, const void* region = text, const size_t region_size = text_size)
{
    uint8_t* addr = reinterpret_cast<uint8_t*>(AOBScanAddress(AOBString, region, region_size));

    if (!addr)
    {
        return nullptr;
    }

    return reinterpret_cast<void*>(addr + Offset);
}

//AOB scan with offset (e.g for when the AOB is inside the function)
inline void* AOBScanCode(const uint8_t* AOBString, const char* AOBMask, const int Offset = 0, const void* region = text, const size_t region_size = text_size)
{
    uint8_t* addr = reinterpret_cast<uint8_t*>(AOBScanAddress(AOBString, AOBMask, region, region_size));

    if (!addr)
    {
        return nullptr;
    }

    return reinterpret_cast<void*>(addr + Offset);
}

//AOB scan for a global pointer [base] through an instruction that uses it
inline void** AOBScanBase(const char* AOBString, const int InOffset = 7, const int OpOffset = 3)
{
    uint8_t* addr = static_cast<uint8_t*>(AOBScanAddress(AOBString));
    return addr != nullptr ? reinterpret_cast<void**>(addr + *reinterpret_cast<int32_t*>(addr + OpOffset) + InOffset) : nullptr;
}

//AOB scan for a global pointer [base] through an instruction that uses it
inline void** AOBScanBase(const unsigned char* AOBString, const char* AOBMask, const int InOffset = 7, const int OpOffset = 3) 
{
    uint8_t* addr = static_cast<uint8_t*>(AOBScanAddress(AOBString, AOBMask));
    return addr != nullptr ? reinterpret_cast<void**>(addr + *reinterpret_cast<int32_t*>(addr + OpOffset) + InOffset) : nullptr;
}

inline void* AOBScanFuncCall(const unsigned char* AOBString1, const char* AOBMask1, const unsigned char* AOBString2, const char* AOBMask2, const size_t FuncSize, const int CallOffset)
{
    uint8_t* addr = reinterpret_cast<uint8_t*>(AOBScanAddress(AOBString1, AOBMask1));

    if (!addr)
    {
        return nullptr;
    }

    addr = addr + *reinterpret_cast<int32_t*>(addr + 1 + CallOffset) + 5 + CallOffset;
    return AOBScanAddress(AOBString2, AOBMask2, addr, FuncSize);
}

//AOB scan for a function through a different function that calls it
inline void* AOBScanCodeCall(const char* AOBString, const int FuncStartToOpOffset, const int OpOffset)
{
    uint8_t* addr = static_cast<uint8_t*>(AOBScanAddress(AOBString));
    return addr != nullptr ? reinterpret_cast<void*>(addr + *reinterpret_cast<int32_t*>(addr + OpOffset + 1) + 5 + FuncStartToOpOffset) : nullptr;
}

//AOB scan for a function through a different function that calls it
inline void* AOBScanCodeCall(const unsigned char* AOBString, const char* AOBMask, const int FuncStartToOpOffset, const int OpOffset)
{
    uint8_t* addr = static_cast<uint8_t*>(AOBScanAddress(AOBString, AOBMask));
    return addr != nullptr ? reinterpret_cast<void*>(addr + *reinterpret_cast<int32_t*>(addr + OpOffset + 1) + 5 + FuncStartToOpOffset) : nullptr;
}

extern void GetText()
{
    const char textStrMatch[] = ".text"; // refused to work with char* string

    const auto pattern = mem::pattern(textStrMatch, ".....");
    const auto region = mem::region(PROCESS_INFO.mInfo.lpBaseOfDll, PROCESS_INFO.mInfo.SizeOfImage);
    uint8_t* result = mem::simd_scanner(pattern).scan(region).any();

    text_size = *reinterpret_cast<uint32_t*>(result + 0x10);
    text = result + *reinterpret_cast<uint32_t*>(result + 0x14);
}

inline void RegisterAddress(const char* name, void** storeTo, void* address)
{
    *storeTo = address;
    registeredAddresses[name] = address;
}


extern void ScanAndAssignAddresses() 
{

    const char* PATTERN_TIMESCALE = "48 8B 05 ?? ?? ?? ?? F3 0F 10 88 ?? ?? ?? ?? F3 0F"; // credits to 'Zullie the Witch' for original offset

    RegisterAddress("PATTERN_TIMESCALE", (void**)&patternTimescale, AOBScanCode(PATTERN_TIMESCALE, 11));

    const char* PATTERN_TIMESCALE_PLAYER = "48 8B 1D ?? ?? ?? ?? 48 85 DB 74 ?? 8B ?? 81 FA"; // credits to 'Zullie the Witch' for original offset

    RegisterAddress("PATTERN_TIMESCALE", (void**)&playerPatternTimescale, AOBScanCode(PATTERN_TIMESCALE_PLAYER, 0));

    const char* hks_lua_typeAOB = "41 8b 10 b8 06 00 00 00 83 e2 0f 8d 4a f7 83 f9 01 0f 47 c2 48 83 c4 30 5b c3";
    const char* hks_luaL_checkoptintAOB = "48 89 5c 24 08 48 89 74 24 10 57 48 83 ec 20 41 8b d8 8b fa 44 8b c2 48 8b f1 48 8b d1 48 8d 4c 24 48 e8 ?? ?? ?? ?? 80 38 00";
    const char* hks_luaL_checkintAOB = "8b d6 48 8b cf e8 ?? ?? ?? ?? 8b c3 48 8b 5c 24 40 48 8b 74 24 48 48 83 c4 30 5f c3";
    const char* hks_luaL_checknumberAOB = "8b d7 48 8b cb e8 ?? ?? ?? ?? 0f 28 f0 0f 57 c9 0f 2e f1 75 ?? 8b d7 48 8b cb";
    const char* hks_luaL_checklstringAOB = "4c 8b c3 8b d6 48 8b cf e8 ?? ?? ?? ?? 48 8b d8 48 85 c0 75";

    const char* getChrInsFromHandleAOB = "48 83 ec 28 e8 ?? ?? ?? ?? 48 85 c0 74 ?? 48 8b 00 48 83 c4 28 c3 48 83 c4 28 c3";
    const char* hks_lua_pushnumberAOB = "48 8b 41 48 f3 0f 11 48 08 c7 00 03 00 00 00 48 83 c0 10 48 89 41 48 c3";
    const char* createHksStateAOB = "48 8b cb e8 ?? ?? ?? ?? 48 8b cb e8 ?? ?? ?? ?? 48 8b cb e8 ?? ?? ?? ?? 48 8b cb e8 ?? ?? ?? ?? 48 8b cb e8 ?? ?? ?? ?? 48 8b cb e8 ?? ?? ?? ?? 48 8b c3";
    const char* hks_addnamedcclosureAOB = "48 89 5c 24 08 57 48 83 ec 30 49 8b c0 c7 44 24 20 00 00 00 00 48 8b da 4c 8b ca 48 8b d0 45 33 c0 48 8b f9";
    const char* hks_lua_pushlstringAOB = "45 8d 88 65 72 6f 6b";

    RegisterAddress("hks_lua_type", (void**)&hks_lua_type, AOBScanCode(hks_lua_typeAOB, -257));
    RegisterAddress("hksHasParamNumberOut", (void**)&hksHasParamNumberOut, AOBScanCodeCall(hks_luaL_checkoptintAOB, 34, 34));
    RegisterAddress("hks_luaL_checkint", (void**)&hks_luaL_checkint, AOBScanCode(hks_luaL_checkintAOB, -123));
    RegisterAddress("hks_luaL_checknumber", (void**)&hks_luaL_checknumber, AOBScanCode(hks_luaL_checknumberAOB, -86));
    RegisterAddress("hks_luaL_checklstring", (void**)&hks_luaL_checklstring, AOBScanCode(hks_luaL_checklstringAOB, -89));
    //RegisterAddress("getEventFlag", (void**)&getEventFlag, AOBScanCode(getEventFlagAOB, -4));
    //RegisterAddress("setEventFlag", (void**)&setEventFlag, AOBScanCode(setEventFlagAOB, -9));
    RegisterAddress("hks_lua_pushnumber", (void**)&hks_lua_pushnumber, AOBScanCode(hks_lua_pushnumberAOB));
    RegisterAddress("hksSetCGlobals", (void**)&hksSetCGlobals, AOBScanCodeCall(createHksStateAOB, 43, 43));
    replacedHksSetCGlobals = hksSetCGlobals;
    RegisterAddress("hks_addnamedcclosure", (void**)&hks_addnamedcclosure, AOBScanCode(hks_addnamedcclosureAOB));
    RegisterAddress("hks_lua_pushlstring", (void**)&hks_lua_pushlstring, AOBScanCode(hks_lua_pushlstringAOB, -25));
}