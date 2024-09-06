#pragma once

#include <Windows.h>
#include <Psapi.h>
#include "ProcessStructs.h"
#include "PointerChain.h"

struct ProcessInfo 
{
    HANDLE hProcess;
    HMODULE hModule;
    MODULEINFO mInfo;
    bool init = false;
};

static ProcessInfo PROCESS_INFO;


static void initBase()
{
    PROCESS_INFO.init = true;
    PROCESS_INFO.hProcess = GetCurrentProcess();
    PROCESS_INFO.hModule = GetModuleHandleA(NULL);
    GetModuleInformation(PROCESS_INFO.hProcess, PROCESS_INFO.hModule, &PROCESS_INFO.mInfo, sizeof(MODULEINFO));
}

intptr_t getProcessBase()
{
    if (!PROCESS_INFO.init) initBase();

    return (intptr_t)PROCESS_INFO.mInfo.lpBaseOfDll;
}

//functions

static const char* CALLER_NAME = "ExposerEventCaller";


//HKS
//+ 0x040de30
/*
* Original hks "env" function.
*/
int (*hksEnv)(void** chrInsPtr, int envId, HksState* hksState);
void* replacedHksEnv;

int (*hksAct)(void** chrInsPtr, int actId, HksState* hksState);
void* replacedHksAct;


//+ 0x149e6a0
int (*hks_lua_type)(HksState* hksState, int idx);

//+ 0x140B910
/*
* Tests if function to be executed has an nth param.
*/
bool* (*hksHasParamNumberOut)(bool* out, void* hksState, int idx);

//bool (*hksHasParam)(void* hksState, int idx);

//+ 0x14A32C0
/*
* Gets the function to be executed's nth param as an int.
*/
int (*hks_luaL_checkint)(HksState* hksState, int idx);

//+ 0xbce940
/*
* Returns the function to be executed's nth param as an int if it exists, otherwise default value.
*/
int (*hks_luaL_checkoptint)(HksState* hksState, int idx, int defaultVal);

//+ 0x14A3370
/*
* Gets the function to be executed's nth param as a float.
*/
float (*hks_luaL_checknumber)(HksState* hksState, int idx);

//+ 0x1497180
const char* (*hks_luaL_checklstring)(HksState* hksState, int idx, size_t* lenOut);

// sekiro time shit
float* patternTimescale;
float* playerPatternTimescale;

/*
* Push number onto the lua stack. (E.g use to return a number in a CFunction).
*/
void (*hks_lua_pushnumber)(HksState* hksState, float number);

void (*hks_lua_pushlstring)(HksState* hksState, const char* str, size_t len);

/*
* Gets hkbCharacter (ptr) owner of the HKS script. This probably returns a pointer to a whole struct whose first element is hkbChr. hkbCharacter->28 is ChrIns
*/
void** (*getHkbChrFromHks)(HksState* hksState);

/*
* Pushes a few globals onto the state, including env and act
*/
void (*hksSetCGlobals)(HksState* hksState);
void* replacedHksSetCGlobals;

/*
* Adds a CFunction to the global table
*/
void (*hks_addnamedcclosure)(HksState* hksState, const char* name, void* func);


bool hksHasParamNumber(HksState* hksState, int paramIndex)
{
    bool out = false;
    return *hksHasParamNumberOut(&out, hksState, paramIndex);
}


/*
* Gets the function to be executed's nth param as a string.
*/
const char* hksGetParamString(HksState* hksState, int paramIndex)
{
    return hks_luaL_checklstring(hksState, paramIndex, NULL);
}

std::string hksParamToString(HksState* hksState, int paramIndex)
{
    int vtype = hks_lua_type(hksState, paramIndex);
    if (vtype == LUA_TSTRING)
        return hksGetParamString(hksState, paramIndex);
    if (vtype == LUA_TNUMBER)
        return std::to_string(hks_luaL_checknumber(hksState, paramIndex)).data();
    if (vtype == LUA_TNONE || vtype == LUA_TNIL)
        return "nil";

    return "Object Type " + vtype;
}

/*
* Same as checkint but if the param is string then convert into long
*/
bool hksGetParamLong(HksState* hksState, int paramIndex, long long& result)
{
    int type = hks_lua_type(hksState, paramIndex);

    if (type == LUA_TNUMBER)
    {
        result = hks_luaL_checknumber(hksState, paramIndex);
        return true;
    }
    else if (type == LUA_TSTRING)
    {
        const char* str = hks_luaL_checklstring(hksState, paramIndex, NULL);
        char* endptr;
        result = strtol(str, &endptr, 10);
        if (endptr == str)
            result = strtod(str, &endptr);
        if (endptr == str) /* conversion failed */
            return false;
        if (*endptr == 'x' || *endptr == 'X')  /* maybe an hexadecimal constant? */
            result = strtoul(str, &endptr, 16);
        if (*endptr == '\0') /* most common case */
            return true;
        while (isspace((unsigned char)*endptr)) endptr++;
        if (*endptr != '\0') /* invalid trailing characters? */
            return false;

        return true;
    }

    return false;
}

/*
* Same as checkint but if the param is string then convert into long
*/
inline long long hksGetParamLong(HksState* hksState, int paramIndex, bool& valid)
{
    long long result = 0;
    valid = hksGetParamLong(hksState, paramIndex, result);
    return result;
}

/*
* Same as checkint but if the param is string then convert into long
*/
inline long long hksGetParamLong(HksState* hksState, int paramIndex)
{
    long long result = 0;
    hksGetParamLong(hksState, paramIndex, result);
    return result;
}

/*
* Same as checkint but if the param is string then convert into int. Bypasses lua's native float conversion first.
*/
inline int hksGetParamInt(HksState* hksState, int paramIndex, bool& valid) 
{
    return (int)hksGetParamLong(hksState, paramIndex, valid);
}

/*
* Same as checkint but if the param is string then convert into int. Bypasses lua's native float conversion first.
*/
inline int hksGetParamInt(HksState* hksState, int paramIndex)
{
    bool valid;
    return (int)hksGetParamInt(hksState, paramIndex, valid);
}

/*
* Same as checknumber but if the param is string then convert into double
*/
bool hksGetParamDouble(HksState* hksState, int paramIndex, double& result)
{
    int type = hks_lua_type(hksState, paramIndex);

    if (type == LUA_TNUMBER)
    {
        result = hks_luaL_checknumber(hksState, paramIndex);
        return true;
    }
    else if (type == LUA_TSTRING)
    {
        const char* str = hks_luaL_checklstring(hksState, paramIndex, NULL);
        char* endptr;
        result = strtod(str, &endptr);
        if (endptr == str) /* conversion failed */
            return false;
        if (*endptr == 'x' || *endptr == 'X')  /* maybe an hexadecimal constant? */
            result = strtoul(str, &endptr, 16);
        if (*endptr == '\0') /* most common case */
            return true;
        while (isspace((unsigned char)*endptr)) endptr++;
        if (*endptr != '\0') /* invalid trailing characters? */
            return false;

        return true;
    }

    return false;
}

/*
* Same as checknumber but if the param is string then convert into double
*/
inline double hksGetParamDouble(HksState* hksState, int paramIndex, bool& valid)
{
    double result = 0;
    valid = hksGetParamDouble(hksState, paramIndex, result);
    return result;
}

/*
* Same as checknumber but if the param is string then convert into double
*/
inline double hksGetParamDouble(HksState* hksState, int paramIndex)
{
    double result = 0;
    hksGetParamDouble(hksState, paramIndex, result);
    return result;
}

void hksPushString(HksState* hksState, std::string str)
{
    hks_lua_pushlstring(hksState, str.c_str(), str.length());
}


inline void hksPushNil(HksState* hksState) 
{
    // I'm not making all lua hks structs
    int* top = *PointerChain::make<int*>(hksState, 0x48);   // top = hksState->top
    *top = LUA_TNIL;                                        // top->tt = LUA_TNIL
    *PointerChain::make<intptr_t>(hksState, 0x48) = ((intptr_t)top) + 0x10;    // hksState->top = top + 1;
};

