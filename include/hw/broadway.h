/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

void broadway_Initialize();
void broadway_Reset();
void broadway_Shutdown();

void broadway_Run();

void broadway_SetEntry(const u32 addr);

void broadway_TryInterrupt();

i64* broadway_GetCyclesToRun();
