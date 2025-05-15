/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#pragma once

#include "common/config.h"

void nouwii_Initialize(const common_Config* config);
void nouwii_Reset();
void nouwii_Shutdown();

void nouwii_Run();
