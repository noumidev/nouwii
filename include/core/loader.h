/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/types.h"

void loader_SetDolPath(const char* path);
void loader_LoadDol();

u32 loader_GetEntry();
