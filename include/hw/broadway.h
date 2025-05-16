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

i64* broadway_GetCyclesToRun();
