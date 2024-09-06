#pragma once
#include <string>
#include "ProcessData.h"
#include "../include/Logger.h"
#include "../include/PointerChain.h"
#include "../game/OtherHooks.h"
#include <iostream>
#include <ctime>

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
/// Function for new acts
/// </summary>
/// <param name="chrInsPtr"></param>
/// <param name="actId"></param>
/// <param name="hksState"></param>
/// <returns></returns>
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
static int urgentExposePrint(HksState* hksState)
{
  // Get the additional parameters for x, y, width, height
  int x = static_cast<int>(hksGetParamDouble(hksState, 1));
  int y = static_cast<int>(hksGetParamDouble(hksState, 2));
  int width = static_cast<int>(hksGetParamDouble(hksState, 3));
  int height = static_cast<int>(hksGetParamDouble(hksState, 4));

  HWND hWnd = GetConsoleWindow(); // get HWND and set the window to the foreground, then move and resize

  if (hWnd == NULL)
  {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    Logger::log("Created Scripts-Data-Exposer-FS Console");
    hWnd = GetConsoleWindow(); // Update after console creation
  }
  ShowWindow(hWnd, SW_RESTORE);
  SetForegroundWindow(hWnd);

  SetWindowPos(hWnd, NULL, x, y, width, height, SWP_SHOWWINDOW | SWP_NOZORDER);

  Logger::log("[Urgent HKS Exposer]: " + hksParamToString(hksState, 5));

  return 0;
}

static int urgentGetTextInput(HksState* hksState)
{
  // Get the additional parameters for x, y, width, height
  int x = static_cast<int>(hksGetParamDouble(hksState, 1));
  int y = static_cast<int>(hksGetParamDouble(hksState, 2));
  int width = static_cast<int>(hksGetParamDouble(hksState, 3));
  int height = static_cast<int>(hksGetParamDouble(hksState, 4));



  HWND hWnd = GetConsoleWindow(); // get HWND and set the window to the foreground, then move and resize

  if (hWnd == NULL)
  {
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    Logger::log("Created Scripts-Data-Exposer-FS Console");
    hWnd = GetConsoleWindow(); // Update after console creation
  }
  ShowWindow(hWnd, SW_RESTORE);
  SetForegroundWindow(hWnd);

  SetWindowPos(hWnd, NULL, x, y, width, height, SWP_SHOWWINDOW | SWP_NOZORDER);

  Logger::log("[Urgent HKS Exposer]: " + hksParamToString(hksState, 5));

  // Get text input from the console
  std::string input;
  std::getline(std::cin, input);

  // Log the input
  Logger::log("[Urgent HKS Exposer]: Received input: " + input);

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

static void newPushEnvActGlobalsFunc(HksState* hksState)
{
    hks_addnamedcclosure(hksState, "exposePrint", exposePrint);
    hks_addnamedcclosure(hksState, "getTextInput", getTextInput);
    hks_addnamedcclosure(hksState, "minimizeConsole", minimizeConsole);
    hks_addnamedcclosure(hksState, "focusConsole", focusConsole);
    hks_addnamedcclosure(hksState, "setConsolePosSize", setConsolePosSize);
    hks_addnamedcclosure(hksState, "setTimeStepSize", setTimeStepSize);
    hks_addnamedcclosure(hksState, "getOSClockLua", getOSClockLua);
}
static void hksSetCGlobalsHookFunc(HksState* hksState)
{
    hksSetCGlobals(hksState);
    newPushEnvActGlobalsFunc(hksState);
}