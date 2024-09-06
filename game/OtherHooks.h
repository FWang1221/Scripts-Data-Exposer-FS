#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <Psapi.h>
#include "HksState.h"
#include "../include/MinHook.h"
#include "../include/Logger.h"
#include "ProcessData.h"
#include "AOBScan.h"

static uintptr_t addressToSet = 0x143D7A359;
typedef __int64(__fastcall* __timeStepSetter)(__int64 trash, __int64 FD4Time);
static __timeStepSetter timeStepSetter = (__timeStepSetter)(0x1411DF900);
static __timeStepSetter timeStepSetterOriginal = NULL;
extern float timeStepper = 0.0166f;
__int64 setTimeStep(__int64 trash, __int64 FD4Time) {

  if (true)
  {
    *(float*)(FD4Time + 0x8) = timeStepper; //normal time step is 1/60.
    unsigned char* byteValuePtr = reinterpret_cast<unsigned char*>(addressToSet);

    // Set the value at the memory address to "1" (binary: 00000001)
    *byteValuePtr |= 0x01;
  }

  return timeStepSetterOriginal(trash, FD4Time);
}

static void otherHooks() {

	MH_CreateHook((LPVOID)timeStepSetter, (LPVOID)&setTimeStep, reinterpret_cast<LPVOID*>(&timeStepSetterOriginal));
}