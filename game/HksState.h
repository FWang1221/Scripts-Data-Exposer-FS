#pragma once
#include <string>
#include "ProcessData.h"
#include "../include/Logger.h"
#include "../include/PointerChain.h"
#include "../game/OtherHooks.h"
#include <iostream>
#include <ctime>
#include <map>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include "../include/httplib.h"
#include <filesystem> // C++17
#include <fstream>
#include <windows.h>

namespace fs = std::filesystem;
extern "C" {
#include "../include/lua.h"
#include "../include/lualib.h"
#include "../include/lauxlib.h"
}
enum EnvId
{
    TRAVERSE_POINTER_CHAIN = 10000,
    EXECUTE_FUNCTION = 10002,
    GET_EVENT_FLAG = 10003,
    GET_PARAM = 10004,
};
enum ActId
{
    WRITE_POINTER_CHAIN = 10000,
    DEBUG_PRINT = 10001,
    UPDATE_MAGICID = 10002,
    SET_EVENT_FLAG = 10003,
    SET_PARAM = 10004,
    CHR_SPAWN_DEBUG = 10005,

    //ESD
    REPLACE_TOOL = 100059,

};

//TODO Change all ugly pointer traversals with PointerChain


//hks functions return invalid when something is wrong, so we'll do the same with our custom funcs
constexpr float INVALID = -1;

static int SINGLE_BIT_MASKS[] = { 1, 2, 4, 8, 16, 32, 64, 128 };


/*
* Helper functions
*/




/*
* Convert model id to name
* Examples:
* 0 = c0000
* 420 = c0420
* 4700 = c4700
*/
#define MODEL_LENGTH (5)
inline wchar_t digitToWChar(char digit)
{
    return digit + 0x30;
}

inline bool modelIdToName(int id, wchar_t* name)
{
    if (id < 0 || id > 9999) return false;

    name[0] = L'c';

    for (int i = 1; i < MODEL_LENGTH; i++)
    {
        char digit = id % 10;
        name[MODEL_LENGTH - i] = digitToWChar(digit);
        id = id / 10;
    }

    return true;
}

inline bool isPlayerIns(void* chrIns)
{
    return ((bool (*)(void*))(*(intptr_t*)(*(intptr_t*)chrIns + 0x118)))(chrIns);
}

/// <summary>
/// Starting offset for pointer traversing functions
/// </summary>
/// <param name="baseType"></param>
/// <param name="hksState"></param>
/// <param name="chrIns"></param>
/// <returns></returns>
inline intptr_t getBaseFromType(PointerBaseType baseType, void* hksState, void* chrIns)
{
  if (true) return getProcessBase(); // temporary fix
  /*
    if (baseType == GAME)
        return getProcessBase();
    if (baseType == CHR_INS)
        return (intptr_t)chrIns;
    if (baseType == TARGET_CHR_INS)
    {
        if (isPlayerIns(chrIns))
        {
            //playerIns->targetHandle
            uint64_t targetHandle = *(uint64_t*)((intptr_t)chrIns + 0x6b0);
            return (intptr_t)getChrInsFromHandle(*WorldChrMan, &targetHandle);
        }
    }

    return 0;*/
}
/*
void* getParamRowEntry(int paramIndex, int rowId)
{
    intptr_t param = (intptr_t)getParamData(paramIndex);
    if (param == NULL) return NULL;

    short rowCount = *(short*)(param + 0xA);
    for (int i = 0; i <= rowCount; i++)
    {
        int currId = *(int*)(param + 0x40 + 0x18 * i);
        if (currId == rowId)
        {
            return (void*)(param + *(int*)(param + 0x48 + 0x18 * i));
        }
        //rows are sorted (I think)
        else if (currId > rowId) return NULL;
    }

    return NULL;
}*/

float getValueFromAddress(intptr_t address, int valueType, char bitOffset = 0)
{
    switch (valueType)
    {
    case UNSIGNED_BYTE_ADDR:
        return (float)*(unsigned char*)address;
    case SIGNED_BYTE_ADDR:
        return (float)*(char*)address;
    case UNSIGNED_SHORT_ADDR:
        return (float)*(unsigned short*)address;
    case SIGNED_SHORT_ADDR:
        return (float)*(short*)address;
    case UNSIGNED_INT_ADDR:
        return (float)*(unsigned int*)address;
    case SIGNED_INT_ADDR:
        return (float)*(int*)address;
    case FLOAT_ADDR:
        return (float)*(float*)address;
    case BIT_ADDR:
        return (float)(((*(unsigned char*)address) & SINGLE_BIT_MASKS[bitOffset]) != 0);
    }

    return (float)*(int*)address;
}

void setValueInAddress(intptr_t address, int valueType, int iValue, float fValue, char bitOffset = 0)
{
    switch (valueType)
    {
    case UNSIGNED_BYTE_ADDR:
        *(unsigned char*)address = (unsigned char)iValue;
        return;
    case SIGNED_BYTE_ADDR:
        *(char*)address = (char)iValue;
        return;
    case UNSIGNED_SHORT_ADDR:
        *(unsigned short*)address = (unsigned short)iValue;
        return;
    case SIGNED_SHORT_ADDR:
        *(short*)address = (short)iValue;
        return;
    case UNSIGNED_INT_ADDR:
        *(unsigned int*)address = (unsigned int)iValue;
        return;
    case SIGNED_INT_ADDR:
        *(int*)address = iValue;
        return;
    case FLOAT_ADDR:
        *(float*)address = fValue;
        return;
    case BIT_ADDR:
        if ((char)iValue == 0)
            *(uint8_t*)(address) = *(uint8_t*)(address) & ~SINGLE_BIT_MASKS[bitOffset];
        else
            *(uint8_t*)(address) = *(uint8_t*)(address) | SINGLE_BIT_MASKS[bitOffset];
        return;
    }
}

//New hook functions

auto NO_ACT = "No act.";
auto OK = "OK.";
auto INCORRECT_ARGS = "Not enough arguments.";
auto NULL_POINTER = "Encountered null pointer.";
auto PARAM_DOESNT_EXIST = "Param doesn't exist.";
auto INCORRECT_MODEL = "Incorrect model format.";

/// <summary>
/// Function for new envs
/// </summary>
/// <param name="chrInsPtr"></param>
/// <param name="envId"></param>
/// <param name="hksState"></param>
/// <returns></returns>
std::pair<const char*, float> newEnvFunc(void** chrInsPtr, int envId, HksState* hksState)
{
    switch (envId)
    {
    case TRAVERSE_POINTER_CHAIN:
    {
        //pointerBaseType, valueType, bitOffset/pointerOffset1, pointerOffsets...)
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4)) 
        {
            return std::pair(INCORRECT_ARGS, INVALID);
        }
        intptr_t address = getBaseFromType((PointerBaseType)hksGetParamInt(hksState, 2), hksState, *chrInsPtr);
        if (address == 0)
            return std::pair(NULL_POINTER, INVALID);

        int valueType = hksGetParamInt(hksState, 3);
        int paramIndex = 4;
        int bitOffset = 0;

        if (valueType == BIT_ADDR)
        {
            if (!hksHasParamNumber(hksState, 5))
                return std::pair(INCORRECT_ARGS, INVALID);

            paramIndex = 5;
            bitOffset = hksGetParamInt(hksState, 4);
        }
        else
            paramIndex = 4;

        while (hksHasParamNumber(hksState, paramIndex + 1))
        {
            if (address == 0)
                return std::pair(NULL_POINTER, INVALID);

            intptr_t offset = hksGetParamLong(hksState, paramIndex);
    

            address = *(intptr_t*)(address + offset);
            paramIndex++;
        }
        if (address == 0)
            return std::pair(NULL_POINTER, INVALID);

        address = address + hksGetParamLong(hksState, paramIndex);

        return std::pair(OK, getValueFromAddress(address, valueType, bitOffset));
    }
    /*case GET_EVENT_FLAG:
    {
        //flagId
        if (!hksHasParamNumber(hksState, 2))
            return std::pair(INCORRECT_ARGS, INVALID);

        return std::pair(OK, getEventFlag(*VirtualMemoryFlag, hksGetParamInt(hksState, 2)));
    }*/
    /*
    case GET_PARAM:
    {
        //paramIndex, row, offset, valueType, <optional> bitOffset
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4) || !hksHasParamNumber(hksState, 5))
            return std::pair(INCORRECT_ARGS, INVALID);

        void* rowEntry = getParamRowEntry(hksGetParamInt(hksState, 2), hksGetParamInt(hksState, 3));
        if (rowEntry == NULL)
            return std::pair(PARAM_DOESNT_EXIST, INVALID);
        intptr_t valAddr = (intptr_t)rowEntry + hksGetParamInt(hksState, 4);


        int valType = hksGetParamInt(hksState, 5);
        int bitOffset = 0;

        if (valType == BIT_ADDR)
        {
            if (!hksHasParamNumber(hksState, 6)) 
                return std::pair(INCORRECT_ARGS, INVALID);
            bitOffset = hksGetParamInt(hksState, 6);
            if (bitOffset > 7) return 
                std::pair(INCORRECT_ARGS, INVALID);
        }

        return std::pair(OK, getValueFromAddress(valAddr, valType, bitOffset));
    }*/

    }

    return std::pair(NO_ACT, INVALID);
}

/// <summary>
/// Function for new acts
/// </summary>
/// <param name="chrInsPtr"></param>
/// <param name="actId"></param>
/// <param name="hksState"></param>
/// <returns></returns>
static const char* newActFunc(void** chrInsPtr, int actId, HksState* hksState)
{
    switch (actId)
    {
    case WRITE_POINTER_CHAIN:
    {
        //base, valueType, value, bitOffset/pointerOffset1, pointerOffsets...
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4) || !hksHasParamNumber(hksState, 5))
            return INCORRECT_ARGS;
        intptr_t address = getBaseFromType((PointerBaseType)hksGetParamInt(hksState, 2), hksState, *chrInsPtr);
        int valType = hksGetParamInt(hksState, 3);
        int paramIndex = 4;
        int bitOffset = 0;

        if (valType == BIT_ADDR)
        {
            if (!hksHasParamNumber(hksState, 6))
                return INCORRECT_ARGS;

            paramIndex = 6;
            bitOffset = hksGetParamInt(hksState, 5);
        }
        else
            paramIndex = 5;

        while (hksHasParamNumber(hksState, paramIndex + 1))
        {
            if (address == 0)
                return NULL_POINTER;

            intptr_t offset = hksGetParamLong(hksState, paramIndex);
            address = *(intptr_t*)(address + offset);
            paramIndex++;
        }
        if (address == 0)
            return NULL_POINTER;
        address = address + hksGetParamLong(hksState, paramIndex);

        setValueInAddress(address, valType, hksGetParamInt(hksState, 4), hks_luaL_checknumber(hksState, 4), bitOffset);
        return OK;
    }
    /*
    case DEBUG_PRINT:
    {
        if (GetConsoleWindow() == NULL)
        {
            AllocConsole();
            freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
            Logger::log("Created Scripts-Data-Exposer-FS Console");
        }
        Logger::log("[HKS Exposer]: " + hksParamToString(hksState, 2));
        return OK;
    }
    */
    /*
    case UPDATE_MAGICID:
    {
        void* chrIns = *chrInsPtr;
        void* (*getPlayerGameData)(void*) = *PointerChain::make<void* (*)(void*)>(chrIns, 0x0, 0x168);
        void* playerGameData = getPlayerGameData(chrIns);
        if (playerGameData == NULL)
            return NULL_POINTER;
        void* equipData = *PointerChain::make<void*>(playerGameData, 0x518);
        if (equipData == NULL)
            return NULL_POINTER;
        int activeSlot = *PointerChain::make<int>(equipData, 0x80);;
        int magicId = *(int*)((intptr_t)equipData + activeSlot * 8 + 0x10);
        void* magicModule = *PointerChain::make<void*>(chrIns, 0x190, 0x60);
        void (*updateMagicId)(void*, int) = *PointerChain::make<void (*)(void*, int)>(magicModule, 0x0, 0x20);

        updateMagicId(magicModule, magicId);
        return OK;
    }*/
    /*
    case SET_EVENT_FLAG:
    {
        //flagId, flagValue
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3))
            return INCORRECT_ARGS;

        setEventFlag(*VirtualMemoryFlag, hksGetParamInt(hksState, 2), hksGetParamInt(hksState, 3) != 0);
        return OK;
    }*/
    /*
    case SET_PARAM:
    {
        //paramIndex, row, offset, valueType, value, <optional> bitOffset
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4) || !hksHasParamNumber(hksState, 5) || !hksHasParamNumber(hksState, 6))
            return INCORRECT_ARGS;

        void* rowEntry = getParamRowEntry(hksGetParamInt(hksState, 2), hksGetParamInt(hksState, 3));
        if (rowEntry == NULL)
            return PARAM_DOESNT_EXIST;

        intptr_t addrToSet = (intptr_t)rowEntry + hksGetParamInt(hksState, 4);
        int valType = hksGetParamInt(hksState, 5);
        int bitOffset = 0;

        if (valType == BIT_ADDR)
        {
            if (!hksHasParamNumber(hksState, 7)) 
                return INCORRECT_ARGS;
            bitOffset = hksGetParamInt(hksState, 7);
            if (bitOffset > 7) 
                return INCORRECT_ARGS;
        }

        setValueInAddress(addrToSet, valType, hksGetParamInt(hksState, 6), hks_luaL_checknumber(hksState, 6), bitOffset);
        return OK;
    }
    */
    //TODO Use function instead of just writing to memory so you can make multiple chrs at once. Unfinished for now.
    /*
    case CHR_SPAWN_DEBUG:
    {
        //model, npcParam, npcThinkParam, posX, posY, posZ, eventEntityId, talkId, charaInitParam
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4) || !hksHasParamNumber(hksState, 5) || !hksHasParamNumber(hksState, 6) || !hksHasParamNumber(hksState, 7))
            return INCORRECT_ARGS;

        ChrSpawnDbgProperties chrProperties;

        int chrId = hksGetParamInt(hksState, 2);
        wchar_t model[6] = { 0 };
        if (!modelIdToName(chrId, model)) //invalid id
            return INCORRECT_MODEL;
        if (chrId == 0)
            chrProperties.isPlayer = true;
        memcpy_s(chrProperties.model, 10, model, 10);

        chrProperties.npcParam = hksGetParamInt(hksState, 3);
        chrProperties.npcThinkParam = hksGetParamInt(hksState, 4);

        chrProperties.posX = hks_luaL_checknumber(hksState, 5);
        chrProperties.posY = hks_luaL_checknumber(hksState, 6);
        chrProperties.posZ = hks_luaL_checknumber(hksState, 7);

        if (hksHasParamNumber(hksState, 8))
            chrProperties.eventEntityId = hksGetParamInt(hksState, 8);
        else
            chrProperties.eventEntityId = 0;

        if (hksHasParamNumber(hksState, 9))
            chrProperties.talkId = hksGetParamInt(hksState, 9);
        else
            chrProperties.talkId = 0;

        if (hksHasParamNumber(hksState, 10))
            chrProperties.charaInitParam = hksGetParamInt(hksState, 10);
        else
            chrProperties.charaInitParam = 0;

        createChrDebug(chrProperties);
        return OK;
    }
    */
    //ESD Functions
    /*
    case REPLACE_TOOL:
    {
        if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3))
            return INCORRECT_ARGS;
        int toReplace = hksGetParamInt(hksState, 2);
        int replaceWith = hksGetParamInt(hksState, 3);
        char unkChar = hksHasParamNumber(hksState, 4) ? hksGetParamInt(hksState, 4) : 1;

        intptr_t chrIns = (intptr_t)*chrInsPtr;
        if (!isPlayerIns((void*)chrIns)) 
            return OK;

        intptr_t gameData = ((intptr_t(*)(intptr_t))(*(intptr_t*)(*(intptr_t*)chrIns + 0x168)))(chrIns);
        if (gameData == NULL) 
            return NULL_POINTER;
        //gameData->equipData

        replaceItem((void*)(gameData + 0x2b0), toReplace, replaceWith, unkChar);
    }*/
    }

    return NO_ACT;
}

/// <summary>
/// The actual lua env cfunction pushed onto the lua state.
/// </summary>
/// <param name="hksState"></param>
/// <returns>number of values returned</returns>
static int LuaHks_env(HksState* hksState)
{
  /*
    void* chrIns = getHksChrInsOwner(hksState);
    if (chrIns == NULL || !hksHasParamNumber(hksState, 1))
    {
        //ChrIns should never be null, there should always be envId (1st argument)
        hks_lua_pushnumber(hksState, 0);
        return 1;
    }*/

    //chrIns->chrModules->behaviorScript->chrEnvRunsOn
    //always self?
    //void** envTarget = *PointerChain::make<void**>(chrIns, 0x190, 0x10, 0x18);
  void** envTarget = nullptr;
    int envId = hks_luaL_checkint(hksState, 1);

    //The function acceptable "envId" are mutually exclusive. The one not used must return 0, so their sum would be the result of the used function
    float numRes = 0;
    auto result = newEnvFunc(envTarget, envId, hksState); 
    if (result.first == OK)
    {
        hks_lua_pushnumber(hksState, result.second);
        return 1;
    }
    else if (result.first == NO_ACT) 
    {
        hks_lua_pushnumber(hksState, (float)hksEnv(envTarget, envId, hksState));
        return 1;
    }
    else
    {
        hksPushNil(hksState);
        hksPushString(hksState, result.first);
        return 2;
    }
    
    return 0;
}

/// <summary>
/// The actual lua act cfunction pushed onto the lua state.
/// </summary>
/// <param name="hksState"></param>
/// <returns>number of values returned</returns>
static int LuaHks_act(HksState* hksState)
{
    //void* chrIns = getHksChrInsOwner(hksState);
    /*if (chrIns == NULL || !hksHasParamNumber(hksState, 1))
    {
        //ChrIns should never be null, there should always be actId (1st argument)
        hks_lua_pushnumber(hksState, 0);
        return 1;
    }*/

    //chrIns->chrModules->behaviorScript->chrActRunsOn
    //always self?
    //void** actTarget = *PointerChain::make<void**>(chrIns, 0x190, 0x10, 0x10);
    void** actTarget = nullptr;
    int actId = hks_luaL_checkint(hksState, 1);

    //The original act function does seemingly intentionally return numbers, ours doesn't (use new env instead if you can).

    const char* result = newActFunc(actTarget, actId, hksState);
    if (result == OK)
    {
        hks_lua_pushnumber(hksState, 0);
        return 1;
    }
    else if (result == NO_ACT) 
    {
        hks_lua_pushnumber(hksState, (float)hksAct(actTarget, actId, hksState));
        return 1;
    }
    else 
    {
        hksPushNil(hksState);
        hksPushString(hksState, result);
        return 2;
    }

    return 0;
}

static int exposePrint(HksState* hksState)
{
  if (GetConsoleWindow() == NULL)
  {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    Logger::log("Created Scripts-Data-Exposer-FS Console");
  }
  Logger::log("[HKS Exposer]: " + hksParamToString(hksState, 1));
  return 0;
}
static int getTextInput(HksState* hksState)
{
  if (GetConsoleWindow() == NULL)
  {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    Logger::log("Created Scripts-Data-Exposer-FS Console");
  }

  Logger::log("[HKS Exposer]: " + hksParamToString(hksState, 1));

  // Get text input from the console
  std::string input;
  std::getline(std::cin, input);

  // Log the input
  Logger::log("[HKS Exposer]: Received input: " + input);

  // Push the input string to the HksState
  hksPushString(hksState, input);

  return 1;
}
static int minimizeConsole(HksState* hksState) {
  HWND hWnd = GetConsoleWindow(); // get HWND and call the function for it lol
  if (hWnd != NULL)
    ShowWindow(hWnd, SW_MINIMIZE);
  return 0;
}
static int focusConsole(HksState* hksState) {
  HWND hWnd = GetConsoleWindow(); // get HWND and call the function for it lol
  if (hWnd != NULL) {
    ShowWindow(hWnd, SW_RESTORE);
    SetForegroundWindow(hWnd);
  }
  return 0;
}
static int setConsolePosSize(HksState* hksState)
{
  // Get the additional parameters for x, y, width, height
  int x = static_cast<int>(hksGetParamDouble(hksState, 1));
  int y = static_cast<int>(hksGetParamDouble(hksState, 2));
  int width = static_cast<int>(hksGetParamDouble(hksState, 3));
  int height = static_cast<int>(hksGetParamDouble(hksState, 4));
  HWND hWnd = GetConsoleWindow(); // get HWND and call the function for it lol
  if (hWnd != NULL)
    SetWindowPos(hWnd, NULL, x, y, width, height, SWP_SHOWWINDOW | SWP_NOZORDER);

  return 0;
}
static int setTimeStepSize(HksState* hksState)
{
  timeStepper = static_cast<float>(hksGetParamDouble(hksState, 1));

  return 0;
}
double getCPUTime() {
  // Return the CPU time in seconds
  return static_cast<double>(clock()) / CLOCKS_PER_SEC;
}
static int getOSClockLua(HksState* hksState) {
  hks_lua_pushnumber(hksState, getCPUTime());
  return 1;
}


static int traversePointerChain(HksState* hksState) {

  if (hksGetParamInt(hksState, 1) != TRAVERSE_POINTER_CHAIN) { // first arg literally doesn't matter, but use TRAVERSE_POINTER_CHAIN anyways so that upgrading isn't a pain
    hksPushNil(hksState);
    return 1;
  }
  //pointerBaseType (currently doesn't work), valueType, bitOffset/pointerOffset1, pointerOffsets...)
  if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4))
  {
    hksPushNil(hksState);
    return 1;
  }

  //intptr_t address = reinterpret_cast<intptr_t>(registeredAddresses[hksGetParamString(hksState, 2)]);
  intptr_t address = getProcessBase();
  if (address == 0) {
    hksPushNil(hksState);
    return 1;
  }

  int valueType = hksGetParamInt(hksState, 3);
  int paramIndex = 4;
  int bitOffset = 0;

  if (valueType == BIT_ADDR)
  {
    if (!hksHasParamNumber(hksState, 5)) {
      hksPushNil(hksState);
      return 1;
    }
    paramIndex = 5;
    bitOffset = hksGetParamInt(hksState, 4);
  }
  else
    paramIndex = 4;

  while (hksHasParamNumber(hksState, paramIndex + 1))
  {
    if (address == 0) {
      hksPushNil(hksState);
      return 1;
    }

    intptr_t offset = hksGetParamLong(hksState, paramIndex);


    address = *(intptr_t*)(address + offset);
    paramIndex++;
  }
  if (address == 0) {
    hksPushNil(hksState);
    return 1;
  }

  address = address + hksGetParamLong(hksState, paramIndex);

  intptr_t offset2 = address - getProcessBase();

  // Convert the offset to a hex string
  std::ostringstream oss;
  oss << "0x" << std::hex << offset2;
  std::string addressStr = oss.str();

  //Logger::log("Address: %s", addressStr.c_str());

  hks_lua_pushnumber(hksState, getValueFromAddress(address, valueType, bitOffset));

  return 1;
}

static int writePointerChain(HksState* hksState) {
  //base, valueType, value, bitOffset/pointerOffset1, pointerOffsets...
  if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4) || !hksHasParamNumber(hksState, 5)) {
    hksPushNil(hksState);
    return 1;
  }
  //intptr_t address = reinterpret_cast<intptr_t>(registeredAddresses[hksGetParamString(hksState, 2)]);
  intptr_t address = getProcessBase();
  int valType = hksGetParamInt(hksState, 3);
  int paramIndex = 4;
  int bitOffset = 0;

  if (valType == BIT_ADDR)
  {
    if (!hksHasParamNumber(hksState, 6)) {
      hksPushNil(hksState);
      return 1;
    }

    paramIndex = 6;
    bitOffset = hksGetParamInt(hksState, 5);
  }
  else
    paramIndex = 5;

  while (hksHasParamNumber(hksState, paramIndex + 1))
  {
    if (address == 0) {
      hksPushNil(hksState);
      return 1;
    }

    intptr_t offset = hksGetParamLong(hksState, paramIndex);
    address = *(intptr_t*)(address + offset);
    paramIndex++;
  }
  if (address == 0) {
    hksPushNil(hksState);
    return 1;
  }
  address = address + hksGetParamLong(hksState, paramIndex);

  intptr_t offset2 = address - getProcessBase();

  // Convert the offset to a hex string
  std::ostringstream oss;
  oss << "0x" << std::hex << offset2;
  std::string addressStr = oss.str();

  //Logger::log("Address: %s", addressStr.c_str());

  setValueInAddress(address, valType, hksGetParamInt(hksState, 4), hks_luaL_checknumber(hksState, 4), bitOffset);

  hksPushNil(hksState);
  return 1;

}

static int traversePointerChainDebug(HksState* hksState) {

  if (hksGetParamInt(hksState, 1) != TRAVERSE_POINTER_CHAIN) { // first arg literally doesn't matter, but use TRAVERSE_POINTER_CHAIN anyways so that upgrading isn't a pain
    hksPushNil(hksState);
    return 1;
  }
  //pointerBaseType (currently doesn't work), valueType, bitOffset/pointerOffset1, pointerOffsets...)
  if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4))
  {
    hksPushNil(hksState);
    return 1;
  }

  //intptr_t address = reinterpret_cast<intptr_t>(registeredAddresses[hksGetParamString(hksState, 2)]);
  intptr_t address = getProcessBase();
  if (address == 0) {
    hksPushNil(hksState);
    return 1;
  }

  int valueType = hksGetParamInt(hksState, 3);
  int paramIndex = 4;
  int bitOffset = 0;

  if (valueType == BIT_ADDR)
  {
    if (!hksHasParamNumber(hksState, 5)) {
      hksPushNil(hksState);
      return 1;
    }
    paramIndex = 5;
    bitOffset = hksGetParamInt(hksState, 4);
  }
  else
    paramIndex = 4;

  while (hksHasParamNumber(hksState, paramIndex + 1))
  {
    if (address == 0) {
      hksPushNil(hksState);
      return 1;
    }

    intptr_t offset = hksGetParamLong(hksState, paramIndex);


    address = *(intptr_t*)(address + offset);
    paramIndex++;
  }
  if (address == 0) {
    hksPushNil(hksState);
    return 1;
  }

  address = address + hksGetParamLong(hksState, paramIndex);

  intptr_t offset2 = address - getProcessBase();

  // Convert the offset to a hex string
  std::ostringstream oss;
  oss << "0x" << std::hex << offset2;
  std::string addressStr = oss.str();

  Logger::log("Address: %s", addressStr.c_str());

  hks_lua_pushnumber(hksState, getValueFromAddress(address, valueType, bitOffset));

  return 1;
}

static int writePointerChainDebug(HksState* hksState) {
  //base, valueType, value, bitOffset/pointerOffset1, pointerOffsets...
  if (!hksHasParamNumber(hksState, 2) || !hksHasParamNumber(hksState, 3) || !hksHasParamNumber(hksState, 4) || !hksHasParamNumber(hksState, 5)) {
    hksPushNil(hksState);
    return 1;
  }
  //intptr_t address = reinterpret_cast<intptr_t>(registeredAddresses[hksGetParamString(hksState, 2)]);
  intptr_t address = getProcessBase();
  int valType = hksGetParamInt(hksState, 3);
  int paramIndex = 4;
  int bitOffset = 0;

  if (valType == BIT_ADDR)
  {
    if (!hksHasParamNumber(hksState, 6)) {
      hksPushNil(hksState);
      return 1;
    }

    paramIndex = 6;
    bitOffset = hksGetParamInt(hksState, 5);
  }
  else
    paramIndex = 5;

  while (hksHasParamNumber(hksState, paramIndex + 1))
  {
    if (address == 0) {
      hksPushNil(hksState);
      return 1;
    }

    intptr_t offset = hksGetParamLong(hksState, paramIndex);
    address = *(intptr_t*)(address + offset);
    paramIndex++;
  }
  if (address == 0) {
    hksPushNil(hksState);
    return 1;
  }
  address = address + hksGetParamLong(hksState, paramIndex);

  intptr_t offset2 = address - getProcessBase();

  // Convert the offset to a hex string
  std::ostringstream oss;
  oss << "0x" << std::hex << offset2;
  std::string addressStr = oss.str();

  Logger::log("Address: %s", addressStr.c_str());

  setValueInAddress(address, valType, hksGetParamInt(hksState, 4), hks_luaL_checknumber(hksState, 4), bitOffset);

  hksPushNil(hksState);
  return 1;

}
static int getScannedAddress(HksState* hksState) {
  // Retrieve the name of the address from Lua state
  std::string aobName = hksParamToString(hksState, 1);

  // Find the address associated with the name in the registeredAddresses map
  auto aobAddress = registeredAddresses.find(aobName);

  // Check if the address was found
  if (aobAddress != registeredAddresses.end()) {
    // Get the process base address
    intptr_t processBase = getProcessBase();
    // Get the target address
    intptr_t address = reinterpret_cast<intptr_t>(aobAddress->second);

    // Calculate the relative offset (address - processBase)
    intptr_t offset = address - processBase;

    // Convert the offset to a hex string
    std::ostringstream oss;
    oss << "0x" << std::hex << offset;
    std::string addressStr = oss.str();

    // Push the offset as a string to Lua
    hksPushString(hksState, addressStr.c_str());
    return 1;
  }

  // Push nil if the address is not found
  hksPushNil(hksState);
  return 1;
}
static int getScannedAddressStatic(HksState* hksState) {
  // Retrieve the name of the address from Lua state
  std::string aobName = hksParamToString(hksState, 1);

  // Find the address associated with the name in the registeredAddresses map
  auto aobAddress = staticOffsets.find(aobName);

  // Check if the address was found
  if (aobAddress != staticOffsets.end()) {

    intptr_t address = static_cast<intptr_t>(aobAddress->second);

    // Convert the offset to a hex string
    std::ostringstream oss;
    oss << "0x" << std::hex << address;
    std::string addressStr = oss.str();

    // Push the offset as a string to Lua
    hksPushString(hksState, addressStr.c_str());
    return 1;
  }

  // Push nil if the address is not found
  hksPushNil(hksState);
  return 1;
}


static intptr_t hexStringToPointer(const std::string& hexString) {
  std::string cleanString = hexString;
  // Remove the '0x' prefix if it exists
  if (cleanString.find("0x") == 0) {
    cleanString.erase(0, 2);
  }
  intptr_t address = 0;
  std::stringstream ss(cleanString);
  ss >> std::hex >> address;

  // Check for parsing errors
  if (ss.fail()) {
    throw std::invalid_argument("Invalid hex string format: " + hexString);
  }
  return address;
}

// Convert an intptr_t value to a hex string
static std::string pointerToHexString(intptr_t value) {
  std::stringstream ss;

  // Format the output as a hex string with "0x" prefix
  ss << "0x" << std::hex << std::uppercase << value;

  return ss.str();
}

static int readPointer(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  //std::cout << "Arg: " << arg << std::endl; // Debug output

  // Get the hex string from the first argument
  const char* hexString = arg.c_str();
  //std::cout << "HexString: " << hexString << std::endl; // Debug output

  // Convert hex string to an address
  intptr_t address = hexStringToPointer(hexString);
  //std::cout << "Converted address: " << std::hex << address << std::endl; // Debug output


  // Compute the actual address
  intptr_t actualAddress = address;
  //std::cout << "Computed actual address: " << std::hex << actualAddress << std::endl; // Debug output

  // Read the QWORD (8 bytes) at the computed address
  int64_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(actualAddress), sizeof(value));
  //std::cout << "Read QWORD value: " << std::hex << value << std::endl; // Debug output

  // No need to subtract the base address to get the offset
  intptr_t offsetValue = value;
  //std::cout << "Offset value: " << std::hex << offsetValue << std::endl; // Debug output

  // Convert the offset value to a hex string
  std::string resultHexString = pointerToHexString(offsetValue);

  // Push the hex string to Lua
  hksPushString(L, resultHexString.c_str());

  return 1; // Number of return values
}



// Function to read a value from memory with given offset and size
template<typename T>
T readMemoryValue(uintptr_t address) {
  T value;
  // Assuming you have a function to read memory safely
  // Here we use memcpy for simplicity, but you should use platform-specific methods
  std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(T));
  return value;
}

// Utility function to write a value to memory with a given address
template<typename T>
void writeMemoryValue(uintptr_t address, T value) {
  // Assuming you have a function to write memory safely
  // Here we use memcpy for simplicity, but you should use platform-specific methods
  std::memcpy(reinterpret_cast<void*>(address), &value, sizeof(T));
}


// Function to read a WORD (2 bytes) from memory and interpret as an integer
static int readSmallInteger(HksState* L) {

  std::string arg = hksParamToString(L, 1);

  // Get the hex string from the first argument
  const char* hexString = arg.c_str();

  // Convert hex string to an address
  uintptr_t address = hexStringToPointer(hexString);


  // Compute the actual address
  uintptr_t actualAddress = address;

  // Read 2 bytes (WORD) from the address
  uint16_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(actualAddress), sizeof(value));

  // Push the integer value to Lua
  hks_lua_pushnumber(L, static_cast<int>(value));

  return 1; // Number of return values
}

// Function to read a DWORD (4 bytes) from memory and interpret as an integer
static int readInteger(HksState* L) {

  std::string arg = hksParamToString(L, 1);

  // Get the hex string from the first argument
  const char* hexString = arg.c_str();

  // Convert hex string to an address
  uintptr_t address = hexStringToPointer(hexString);

  uintptr_t actualAddress = address;

  // Read 4 bytes (DWORD) from the address
  uint32_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(actualAddress), sizeof(value));

  // Push the integer value to Lua
  hks_lua_pushnumber(L, static_cast<int>(value));

  return 1; // Number of return values
}


// Function to read an unsigned small integer (WORD, 2 bytes)
static int readUnsignedSmallInteger(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  uint16_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));

  hks_lua_pushnumber(L, static_cast<unsigned int>(value));
  return 1;
}

// Function to read an unsigned integer (DWORD, 4 bytes)
static int readUnsignedInteger(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  uint32_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));

  hks_lua_pushnumber(L, static_cast<unsigned int>(value));
  return 1;
}

// Function to read a float (4 bytes)
static int readFloat(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  float value = 0.0f;
  std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));

  hks_lua_pushnumber(L, static_cast<double>(value));
  return 1;
}

// Function to read a byte (1 byte)
static int readByte(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  int8_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));

  hks_lua_pushnumber(L, static_cast<int>(value));
  return 1;
}

// Function to read an unsigned byte (1 byte)
static int readUnsignedByte(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  uint8_t value = 0;
  std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));

  hks_lua_pushnumber(L, static_cast<unsigned int>(value));
  return 1;
}

// Function to write a small integer (WORD, 2 bytes)
static int writeSmallInteger(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  int value = hksGetParamInt(L, 2);
  uint16_t smallValue = static_cast<uint16_t>(value);

  writeMemoryValue(address, smallValue);

  return 0; // No return values
}

// Function to write an unsigned small integer (WORD, 2 bytes)
static int writeUnsignedSmallInteger(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  unsigned int value = hksGetParamInt(L, 2);
  uint16_t smallValue = static_cast<uint16_t>(value);

  writeMemoryValue(address, smallValue);

  return 0;
}

// Function to write an integer (DWORD, 4 bytes)
static int writeInteger(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  int value = hksGetParamInt(L, 2);

  writeMemoryValue(address, value);

  return 0;
}

// Function to write an unsigned integer (DWORD, 4 bytes)
static int writeUnsignedInteger(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  unsigned int value = hksGetParamInt(L, 2);

  writeMemoryValue(address, value);

  return 0;
}

// Function to write a float (4 bytes)
static int writeFloat(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  float value = static_cast<float>(hksGetParamInt(L, 2));

  writeMemoryValue(address, value);

  return 0;
}

// Function to write a byte (1 byte)
static int writeByte(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  int value = hksGetParamInt(L, 2);
  int8_t byteValue = static_cast<int8_t>(value);

  writeMemoryValue(address, byteValue);

  return 0;
}

// Function to write an unsigned byte (1 byte)
static int writeUnsignedByte(HksState* L) {
  std::string arg = hksParamToString(L, 1);
  const char* hexString = arg.c_str();
  uintptr_t address = hexStringToPointer(hexString);

  unsigned int value = hksGetParamInt(L, 2);
  uint8_t byteValue = static_cast<uint8_t>(value);

  writeMemoryValue(address, byteValue);

  return 0;
}

static int getProcessBaseHexLua(HksState* hksState) {
  uintptr_t processBase = getProcessBase();

  std::string resultHexString = pointerToHexString(processBase);

  // Push the hex string to Lua
  hksPushString(hksState, resultHexString.c_str());

  return 1;
}
// Static Lua state
static lua_State* L = nullptr;

static void initialize_lua_state() {
  if (L) {
    Logger::debug("Lua state already initialized!");
    return;
  }
  L = luaL_newstate();
  if (!L) {
    Logger::debug("Failed to create Lua state!");
    return;
  }
  luaL_openlibs(L); // Open Lua libraries
  Logger::debug("Lua state initialized!");
}

static void run_lua_file(const char* filename) {
  if (!L) {

    Logger::debug("Lua state not initialized!");
    return;
  }
  if (luaL_dofile(L, filename) != 0) {
    Logger::debug("Error loading Lua file: %s", lua_tostring(L, -1));
    lua_pop(L, 1); // Remove error message from stack
  }
}

static void cleanup_lua_state() {
  if (L) {
    lua_close(L);
    L = nullptr;
  }
}

// Wrapper function to expose to Lua
static int lua_run_lua_file(HksState* hksState) {
  std::string arg = hksParamToString(hksState, 1);
  const char* filename = arg.c_str();
  if (luaL_dofile(L, filename) != 0) {
    Logger::debug("Error loading Lua file: %s\n\n%s", filename, lua_tostring(L, -1));
    return 0; // Return error message
  }
  return 0; // No error
}

// Wrapper function for plain text
static int lua_run_lua_code(HksState* hksState) {
  std::string arg = hksParamToString(hksState, 1);
  const char* code = arg.c_str();
  if (luaL_dostring(L, code) != 0) {
    Logger::debug("Error loading Lua code: %s\n\n%s", code, lua_tostring(L, -1));
    return 0; // Return error message
  }
  return 0; // No error
}

// Wrapper function to get JSON encoded global variable
static int lua_get_json_encoded_global(HksState* hksState) {
  std::string arg = hksParamToString(hksState, 1);
  const char* var_name = arg.c_str();

  lua_getglobal(L, var_name); // Push the global variable onto the stack

  if (lua_isnil(L, -1)) {
    Logger::debug("Global variable is nil");
    return 0;
  }

  // Call json.encode()
  lua_getglobal(L, "json");
  lua_getfield(L, -1, "encode"); // Get json.encode function
  lua_pushvalue(L, -3); // Push the value to be encoded

  if (lua_pcall(L, 1, 1, 0) != 0) {
    Logger::debug("Error getting global: %s", lua_tostring(L, -1));
    lua_pop(L, 2); // Pop the result and json library
    return 0;
  }
  const char* json_encoded_str = lua_tostring(L, -1); // Get the JSON encoded string

  hksPushString(hksState, json_encoded_str);

  return 1; // Return the encoded JSON string
}
// local status, response = makeHttpGetRequest(url, body)
static int makeHttpGetRequest(HksState* hksState) {
  // Get the URL and body parameters
  std::string url = hksParamToString(hksState, 1);  // First argument: URL
  std::string body = hksParamToString(hksState, 2); // Second argument: Body (optional, not typically used for GET)

  // Split URL into host and path using httplib
  httplib::Client cli(url.c_str());

  // Make the GET request
  auto res = cli.Get("/"); // Assuming you want to request the root path

  if (res) {
    hks_lua_pushnumber(hksState, res->status);
    hksPushString(hksState, res->body);
    return 2;
  }
  else {
    Logger::debug("Request failed.");
    return 0;
  }

  return 0;
}

// local status, response = makeHttpPostRequest(url, body)
static int makeHttpPostRequest(HksState* hksState) {
  // Get the URL and body parameters
  std::string url = hksParamToString(hksState, 1);  // First argument: URL
  std::string body = hksParamToString(hksState, 2); // Second argument: Body

  // Create the client using the URL
  httplib::Client cli(url.c_str());

  // Send the POST request to the root path ("/"), including the body
  auto res = cli.Post("/", body, "application/x-www-form-urlencoded"); // You can change the content type if needed

  if (res) {
    // Push the response status code and body to Lua
    hks_lua_pushnumber(hksState, res->status);
    hksPushString(hksState, res->body);
    return 2; // Returning 2 values (status and body)
  }
  else {
    Logger::debug("Request failed.");
    return 0; // No return in case of failure
  }

  return 0;
}


// File size
static int luaFileSize(HksState* hksState) {
  std::string filepath = hksParamToString(hksState, 1); // Get filepath from Lua
  try {
    auto size = fs::file_size(filepath);
    hks_lua_pushnumber(hksState, size);  // Push file size as number
  }
  catch (fs::filesystem_error& e) {
    Logger::debug("Error: %s", e.what());
    return 0; // Return no value in case of error
  }
  return 1; // Return one value (file size)
}

// File permissions
static int luaFilePermissions(HksState* hksState) {
  std::string filepath = hksParamToString(hksState, 1); // Get filepath from Lua
  try {
    auto perms = fs::status(filepath).permissions();
    hks_lua_pushnumber(hksState, static_cast<int>(perms));  // Push permissions as number
  }
  catch (fs::filesystem_error& e) {
    Logger::debug("Error: %s", e.what());
    return 0; // Return no value in case of error
  }
  return 1; // Return one value (permissions)
}

// Last file access time
static int luaLastFileAccess(HksState* hksState) {
  std::string filepath = hksParamToString(hksState, 1); // Get filepath from Lua
  try {
    auto ftime = fs::last_write_time(filepath);
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
    hks_lua_pushnumber(hksState, seconds);  // Push last access time as a timestamp
  }
  catch (fs::filesystem_error& e) {
    Logger::debug("Error: %s", e.what());
    return 0; // Return no value in case of error
  }
  return 1; // Return one value (timestamp)
}

// List all files/folders within a directory (separated by "|")
static int luaListFilesInDir(HksState* hksState) {
  std::string dirpath = hksParamToString(hksState, 1); // Get directory path from Lua
  std::string result;

  try {
    for (const auto& entry : fs::directory_iterator(dirpath)) {
      result += entry.path().string() + "|"; // Use "|" as separator
    }
    if (!result.empty()) {
      result.pop_back(); // Remove the last "|"
    }
    hksPushString(hksState, result);  // Push the list of files/folders as a single string
  }
  catch (fs::filesystem_error& e) {
    Logger::debug("Error: %s", e.what());
    return 0; // Return no value in case of error
  }
  return 1; // Return one value (the file list)
}


#define HKS_SUBDIR L"Software\\HKS_ScriptExposer"

std::wstring stringToWString(const std::string& str) {
  // Find the size needed for the wide string
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);

  // Create a wstring with the needed size
  std::wstring wstr(size_needed, 0);

  // Convert string to wide string
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);

  return wstr;
}

std::string wstringToString(const std::wstring& wstr) {
  // Determine the size needed for the resulting string
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);

  // Create a string with the needed size
  std::string str(size_needed, 0);

  // Convert the wstring to string
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &str[0], size_needed, NULL, NULL);

  return str;
}


// Registry read function
static int luaRegistryRead(HksState* hksState) {
  std::wstring key = stringToWString(hksParamToString(hksState, 1)); // Full registry key path
  std::wstring valueName = stringToWString(hksParamToString(hksState, 2)); // Value name

  HKEY hKey;
  DWORD bufferSize = 1024;
  wchar_t buffer[1024] = { 0 };
  DWORD dwType = REG_SZ; // Assuming we deal with string values

  if (RegOpenKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &dwType, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
      std::wstring result(buffer);
      RegCloseKey(hKey);
      hksPushString(hksState, std::string(result.begin(), result.end())); // Convert to std::string
      return 1; // Return one value (string)
    }
    RegCloseKey(hKey);
  }

  Logger::debug("Registry read failed for key: %s", wstringToString(key));
  return 0; // Return no value in case of failure
}

// Helper function to verify that key is within HKS_ScriptExposer
bool isKeyInHKSSubdir(const std::wstring& key) {
  std::wstring allowedPrefix = L"Software\\HKS_ScriptExposer";
  return key.find(allowedPrefix) == 0;
}

// Registry write function (restricted to HKS_ScriptExposer)
static int luaRegistryWrite(HksState* hksState) {
  std::wstring key = stringToWString(hksParamToString(hksState, 1)); // Full registry key path
  std::wstring valueName = stringToWString(hksParamToString(hksState, 2)); // Value name
  std::wstring value = stringToWString(hksParamToString(hksState, 3)); // Value data

  // Ensure key is inside the allowed subdirectory
  if (!isKeyInHKSSubdir(key)) {
    Logger::debug("Registry read failed for key: %s", wstringToString(key));
    return 0;
  }

  HKEY hKey;
  DWORD disposition;

  // Open or create the key in HKS_ScriptExposer subdirectory
  if (RegCreateKeyExW(HKEY_CURRENT_USER, key.c_str(), 0, NULL, 0, KEY_WRITE, NULL, &hKey, &disposition) == ERROR_SUCCESS) {
    if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (value.size() + 1) * sizeof(wchar_t)) == ERROR_SUCCESS) {
      RegCloseKey(hKey);
      hks_lua_pushnumber(hksState, 1); // Return true for success
      return 1;
    }
    RegCloseKey(hKey);
  }

  Logger::debug("Registry read failed for key: %s", wstringToString(key));
  return 0;
}

static void pushConsoleFuncs(HksState* hksState) // windows console shit
{
  hks_addnamedcclosure(hksState, "exposePrint", exposePrint);
  hks_addnamedcclosure(hksState, "getTextInput", getTextInput);
  hks_addnamedcclosure(hksState, "minimizeConsole", minimizeConsole);
  hks_addnamedcclosure(hksState, "focusConsole", focusConsole);
  hks_addnamedcclosure(hksState, "setConsolePosSize", setConsolePosSize);

}
static void pushExtendedFuncs(HksState* hksState) //clock and file io
{
  hks_addnamedcclosure(hksState, "getOSClockLua", getOSClockLua);
  //implement c file io shit
  hks_addnamedcclosure(hksState, "luaFileSize", luaFileSize);
  hks_addnamedcclosure(hksState, "luaFilePermissions", luaFilePermissions);
  hks_addnamedcclosure(hksState, "luaLastFileAccess", luaLastFileAccess);
  hks_addnamedcclosure(hksState, "luaListFilesInDir", luaListFilesInDir);
  // external funcs and hooks
  hks_addnamedcclosure(hksState, "setTimeStepSize", setTimeStepSize);

}
static void pushLegacyScriptExposerFuncs(HksState* hksState) //old shit that i mangled
{
  //hks_addnamedcclosure(hksState, "env", LuaHks_env);
  //hks_addnamedcclosure(hksState, "act", LuaHks_act);
  hks_addnamedcclosure(hksState, "traversePointerChain", traversePointerChain);
  hks_addnamedcclosure(hksState, "writePointerChain", writePointerChain);
  hks_addnamedcclosure(hksState, "traversePointerChainDebug", traversePointerChainDebug);
  hks_addnamedcclosure(hksState, "writePointerChainDebug", writePointerChainDebug);

}
static void pushMemoryFuncs(HksState* hksState) //unsafe memory weird bullshit, basic stuff
{
  hks_addnamedcclosure(hksState, "getScannedAddress", getScannedAddress);
  hks_addnamedcclosure(hksState, "getScannedAddressStatic", getScannedAddressStatic);
  hks_addnamedcclosure(hksState, "readPointerFunc", readPointer);
  hks_addnamedcclosure(hksState, "readIntegerFunc", readInteger);
  hks_addnamedcclosure(hksState, "readSmallIntegerFunc", readSmallInteger);
  hks_addnamedcclosure(hksState, "readUnsignedSmallIntegerFunc", readUnsignedSmallInteger);
  hks_addnamedcclosure(hksState, "readUnsignedIntegerFunc", readUnsignedInteger);
  hks_addnamedcclosure(hksState, "readFloatFunc", readFloat);
  hks_addnamedcclosure(hksState, "readByteFunc", readByte);
  hks_addnamedcclosure(hksState, "readUnsignedByteFunc", readUnsignedByte);
  hks_addnamedcclosure(hksState, "writeSmallIntegerFunc", writeSmallInteger);
  hks_addnamedcclosure(hksState, "writeUnsignedSmallIntegerFunc", writeUnsignedSmallInteger);
  hks_addnamedcclosure(hksState, "writeIntegerFunc", writeInteger);
  hks_addnamedcclosure(hksState, "writeUnsignedIntegerFunc", writeUnsignedInteger);
  hks_addnamedcclosure(hksState, "writeFloatFunc", writeFloat);
  hks_addnamedcclosure(hksState, "writeByteFunc", writeByte);
  hks_addnamedcclosure(hksState, "writeUnsignedByteFunc", writeUnsignedByte);
  hks_addnamedcclosure(hksState, "getProcessBase", getProcessBaseHexLua);
  //todo: implement memcpy and dumping memory and returning bytes
}
static void pushSecondLuaEnvFuncs(HksState* hksState)
{
    hks_addnamedcclosure(hksState, "secondEnvRunFile", lua_run_lua_file);
    hks_addnamedcclosure(hksState, "secondEnvRunCode", lua_run_lua_code);
    hks_addnamedcclosure(hksState, "secondEnvGetGlobalJSON", lua_get_json_encoded_global);
}
static void pushWebFuncs(HksState* hksState)
{
  //todo: implement GET/POST, as well as server which pushes things to the second lua environment
  //todo: also implement a websocket client/server coz why the fuck not

  hks_addnamedcclosure(hksState, "makeHttpGetRequest", makeHttpGetRequest);
  hks_addnamedcclosure(hksState, "makeHttpPostRequest", makeHttpPostRequest);

}
static void pushRegistryFuncs(HksState* hksState)
{
  //todo: implement get/set registry values

  hks_addnamedcclosure(hksState, "luaRegistryRead", luaRegistryRead);
  hks_addnamedcclosure(hksState, "luaRegistryWrite", luaRegistryWrite);

}
static void pushDirectXDrawFuncs(HksState* hksState)
{
  //todo: store directx drawing objs and hook it and do stuff or something
}
static void pushCallAssemblyFuncs(HksState* hksState)
{
  //todo: allow lua to call funcs similar to cheat engine + also malloc  and mov and other assembly shit
}

static void hksSetCGlobalsHookFunc(HksState* hksState)
{
    hksSetCGlobals(hksState);
    pushConsoleFuncs(hksState);
    pushExtendedFuncs(hksState); // unfinished
    pushLegacyScriptExposerFuncs(hksState);
    pushMemoryFuncs(hksState); // unfinished
    pushSecondLuaEnvFuncs(hksState);
    pushWebFuncs(hksState);
    pushRegistryFuncs(hksState);
    pushDirectXDrawFuncs(hksState); // unfinished
    pushCallAssemblyFuncs(hksState); // unfinished

}