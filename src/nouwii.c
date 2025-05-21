/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "nouwii.h"

#include <stdio.h>

#include "common/config.h"

#include "core/memory.h"

#include "hw/ai.h"
#include "hw/broadway.h"
#include "hw/di.h"
#include "hw/dsp.h"
#include "hw/exi.h"
#include "hw/hollywood.h"
#include "hw/mi.h"
#include "hw/pi.h"

#define NUM_ARGS (1 + 2)

#define MAX_CYCLES_TO_RUN (1024)

void nouwii_Initialize(const common_Config* config) {
    memory_Initialize(config->pathMem1, config->pathMem2);

    ai_Initialize();
    broadway_Initialize();
    di_Initialize();
    dsp_Initialize();
    exi_Initialize();
    hollywood_Initialize();
    mi_Initialize();
    pi_Initialize();
}

void nouwii_Reset() {
    memory_Reset();

    ai_Reset();
    broadway_Reset();
    di_Reset();
    dsp_Reset();
    exi_Reset();
    hollywood_Reset();
    mi_Reset();
    pi_Reset();
}

void nouwii_Shutdown() {
    memory_Shutdown();

    ai_Shutdown();
    broadway_Shutdown();
    di_Shutdown();
    dsp_Shutdown();
    exi_Shutdown();
    hollywood_Shutdown();
    mi_Shutdown();
    pi_Shutdown();
}

void nouwii_Run() {
    while (NOUWII_TRUE) {
        *broadway_GetCyclesToRun() = MAX_CYCLES_TO_RUN;

        broadway_Run();
    }
}

int main(int argc, char** argv) {
    if (argc < NUM_ARGS) {
        puts("Usage: nouwii [path to MEM1 dump] [path to MEM2 dump]");
        return 1;
    }

    common_Config config;
    config.pathMem1 = argv[1];
    config.pathMem2 = argv[2];

    nouwii_Initialize(&config);
    nouwii_Reset();
    nouwii_Run();
    nouwii_Shutdown();

    return 0;
}
