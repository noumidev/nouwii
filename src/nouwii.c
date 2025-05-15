/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "nouwii.h"

#include <stdio.h>

#include "common/config.h"
#include "core/memory.h"

#define NUM_ARGS (1 + 2)

void nouwii_Initialize(const common_Config* config) {
    memory_Initialize(config->pathMem1, config->pathMem2);
}

void nouwii_Reset() {
    memory_Reset();
}

void nouwii_Shutdown() {
    memory_Shutdown();
}

void nouwii_Run() {

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
