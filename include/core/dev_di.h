/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

void dev_di_Initialize();
void dev_di_Reset();
void dev_di_Shutdown();

// vDevice functions
u32 dev_di_Ioctl(const u32 ioctl, const u32 addr0, const u32 size0, const u32 addr1, const u32 size1);
