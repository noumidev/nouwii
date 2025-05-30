/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

typedef void (*scheduler_Callback)(const int);

void scheduler_Initialize();
void scheduler_Reset();
void scheduler_Shutdown();

void scheduler_ScheduleEvent(const char* name, scheduler_Callback callback, const int arg, const i64 cycles);

void scheduler_Run();
