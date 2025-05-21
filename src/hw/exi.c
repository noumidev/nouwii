/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/exi.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CHANNELS (3)

#define SIZE_CHANNEL (0x14)

#define MAKEFUNC_EXI_READIO(size)                                     \
u##size exi_ReadIo##size(const u32 addr) {                            \
    printf("EXI Unimplemented read%d (address: %08X)\n", size, addr); \
    exit(1);                                                          \
}                                                                     \

#define MAKEFUNC_EXI_WRITEIO(size)                                                       \
void exi_WriteIo##size(const u32 addr, const u##size data) {                             \
    printf("EXI Unimplemented write%d (address: %08X, data: %02X)\n", size, addr, data); \
    exit(1);                                                                             \
}                                                                                        \

enum {
    EXI_CSR  = 0xD006800,
    EXI_CR   = 0xD00680C,
    EXI_DATA = 0xD006810,
};

#define  CSR (chn->csr)
#define   CR (chn->cr)
#define DATA (chn->data)

typedef struct Channel {
    union {
        u32 raw;

        struct {
            u32 exiintmask :  1; // EXI interrupt mask
            u32     exiint :  1; // EXI interrupt status
            u32  tcintmask :  1; // Transfer complete interrupt mask
            u32      tcint :  1; // Transfer complete interrupt status
            u32        clk :  3; // Clock
            u32         cs :  3; // Channel select
            u32     extint :  1; // External interrupt
            u32        ext :  1; // External device connected
            u32     romdis :  1; // GC boot ROM disable
            u32            : 19;
        };
    } csr;

    union {
        u32 raw;

        struct {
            u32 tstart :  1;
            u32    dma :  1;
            u32     rw :  2;
            u32   tlen :  2;
            u32        : 26;
        };
    } cr;

    u32 data;
} Channel;

static Channel chns[NUM_CHANNELS];

void exi_Initialize() {

}

void exi_Reset() {
    memset(chns, 0, sizeof(chns));
}

void exi_Shutdown() {

}

MAKEFUNC_EXI_READIO(8)
MAKEFUNC_EXI_READIO(16)
MAKEFUNC_EXI_READIO(64)

u32 exi_ReadIo32(const u32 addr) {
    const int c = (addr - EXI_CSR) / SIZE_CHANNEL;

    Channel* chn = &chns[c];

    switch (EXI_CSR + ((addr - EXI_CSR) % SIZE_CHANNEL)) {
        case EXI_CSR:
            printf("EXI_CSR%d read32\n", c);

            return CSR.raw;
        case EXI_CR:
            printf("EXI_CR%d read32\n", c);

            return CR.raw;
        default:
            printf("EXI Unimplemented read32 (address: %08X)\n", addr);

            exit(1);
    }
}

MAKEFUNC_EXI_WRITEIO(8)
MAKEFUNC_EXI_WRITEIO(16)
MAKEFUNC_EXI_WRITEIO(64)

void exi_WriteIo32(const u32 addr, const u32 data) {
    const int c = (addr - EXI_CSR) / SIZE_CHANNEL;

    Channel* chn = &chns[c];

    switch (EXI_CSR + ((addr - EXI_CSR) % SIZE_CHANNEL)) {
        case EXI_CSR:
            printf("EXI_CSR%d write32 (data: %08X)\n", c, data);

            CSR.raw = data;
            break;
        case EXI_CR:
            printf("EXI_CR%d write32 (data: %08X)\n", c, data);

            CR.raw = data;

            if (CR.tstart != 0) {
                assert(CR.dma == 0);

                printf("EXI channel %d immediate transfer (length: %u, data: %08X, rw: %u)\n", c, CR.tlen + 1, DATA, CR.rw);

                CR.tstart = 0;
            }
            break;
        case EXI_DATA:
            printf("EXI_DATA%d write32 (data: %08X)\n", c, data);

            DATA = data;
            break;
        default:
            printf("EXI Unimplemented write32 (address: %08X, data: %08X)\n", addr, data);

            exit(1); 
    }
}
