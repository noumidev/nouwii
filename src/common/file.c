/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "common/file.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/types.h"

long common_LoadFile(const char* path, void** out) {
    assert(out != NULL);

    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        printf("Unable to open file \"%s\"\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    *out = malloc(size);

    fread(*out, sizeof(u8), size, file);
    fclose(file);

    return size;
}
