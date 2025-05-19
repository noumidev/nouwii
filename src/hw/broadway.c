/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/broadway.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/types.h"
#include "core/memory.h"

#define BROADWAY_DEBUG

#define NUM_GPRS (32)
#define NUM_BATS (8)

#define INITIAL_PC (0x3400)

enum {
    PRIMARY_ADDI     = 14,
    PRIMARY_ADDIS    = 15,
    PRIMARY_SYSTEM   = 19,
    PRIMARY_ORI      = 24,
    PRIMARY_REGISTER = 31,
    PRIMARY_LWZ      = 32,
    PRIMARY_STW      = 36,
};

enum {
    SECONDARY_MFMSR =  83,
    SECONDARY_MTMSR = 146,
    SECONDARY_MTSR  = 210,
    SECONDARY_MFSPR = 339,
    SECONDARY_MTSPR = 467,
};

enum {
    SYSTEM_ISYNC = 150,
};

enum {
    SPR_IBAT0U =  528,
    SPR_IBAT3L =  535,
    SPR_DBAT0U =  536,
    SPR_DBAT3L =  543,
    SPR_IBAT4U =  560,
    SPR_IBAT7L =  567,
    SPR_DBAT4U =  568,
    SPR_DBAT7L =  575,
    SPR_HID0   = 1008,
};

#define MAKEFUNC_BROADWAY_READ(size)                        \
static u##size Read##size(const u32 addr, const int code) { \
    assert((addr & ((size / 8) - 1)) == 0);                 \
    return memory_Read##size(Translate(addr, code));        \
}                                                           \

#define MAKEFUNC_BROADWAY_WRITE(size)                         \
static void Write##size(const u32 addr, const u##size data) { \
    assert((addr & ((size / 8) - 1)) == 0);                   \
    memory_Write##size(Translate(addr, NOUWII_FALSE), data);  \
}                                                             \

#define TO_IBM_POS(n) (31 - n)

static u32 GetBits(const u32 n, const u32 start, const u32 end) {
    const u32 mask = (0xFFFFFFFF << TO_IBM_POS(end)) & (0xFFFFFFFF >> start);

    return (n & mask) >> TO_IBM_POS(end);
}

// Instruction fields
#define OPCD (GetBits(instr,  0,  5))
#define   XO (GetBits(instr, 21, 30))
#define   RA (GetBits(instr, 11, 15))
#define   RB (GetBits(instr, 16, 20))
#define   RD (GetBits(instr,  6, 10))
#define   RS (GetBits(instr,  6, 10))
#define UIMM (GetBits(instr, 16, 31))
#define  SPR (GetBits(instr, 11, 15) | (GetBits(instr, 16, 20) << 5))

// Address fields
#define ADDR_OFFSET  (addr & 0x0001FFFF)
#define ADDR_PAGE    (addr & 0x0FFE0000)
#define ADDR_SEGMENT (addr & 0xF0000000)

// SPRs
#define  HID0 (ctx.sprs.hid0)
#define DBATL (ctx.sprs.dbatl)
#define DBATU (ctx.sprs.dbatu)
#define IBATL (ctx.sprs.ibatl)
#define IBATU (ctx.sprs.ibatu)

#define MSR (ctx.msr)

typedef union Batl {
    u32 raw;

    struct {
        u32   pp :  2; // Protection bits
        u32      :  1;
        u32    g :  1; // Guarded
        u32    m :  1; // Memory coherence
        u32    i :  1; // Inhibited
        u32    w :  1; // Write-through
        u32      : 10;
        u32 brpn : 15; // Block real page number
    };
} Batl;

typedef union Batu {
    u32 raw;

    struct {
        u32   vp :  1; // User valid
        u32   vs :  1; // Supervisor valid
        u32   bl : 11; // Block length
        u32      :  4;
        u32 bepi : 15; // Block effective page index
    };
} Batu;

typedef struct SpecialRegs {
    union {
        u32 raw;

        struct {
            u32 noopti : 1; // No-op touch instructions
            u32        : 1;
            u32    bht : 1; // Branch history table enable
            u32    abe : 1; // Address broadcast enable
            u32        : 1;
            u32   btic : 1; // Branch target icache enable
            u32   dcfa : 1; // Dcache flush assist
            u32    sge : 1; // Store gathering enable
            u32   ifem : 1; // Instruction fetch M enable
            u32    spd : 1; // Speculative cache access disable
            u32   dcfi : 1; // Dcache flash invalidate (auto-clear after setting)
            u32   icfi : 1; // Icache flash invalidate (auto-clear after setting)
            u32  dlock : 1; // Dcache lock
            u32  ilock : 1; // Icache lock
            u32    dce : 1; // Dcache enable
            u32    ice : 1; // Icache enable
            u32    nhr : 1; // Not hard reset
            u32        : 3;
            u32    dpm : 1; // Dynamic power management
            u32  sleep : 1; // Sleep mode enable
            u32    nap : 1; // Nap mode enable
            u32   doze : 1; // Doze mode enable
            u32    par : 1; // Disable precharge
            u32   eclk : 1; // CLK_OUT enable
            u32        : 1;
            u32   bclk : 1; // CLK_OUT enable
            u32    ebd : 1; // Enable data parity checking
            u32    eba : 1; // Enable address parity checking
            u32    dbp : 1; // Disable bus parity generation
            u32   emcp : 1; // Enable MCP
        };
    } hid0;

    Batl dbatl[NUM_BATS], ibatl[NUM_BATS];
    Batu dbatu[NUM_BATS], ibatu[NUM_BATS];
} SpecialRegs;

typedef struct Context {
    i64 cyclesToRun;

    u32 ia, cia;

    u32 r[NUM_GPRS];

    SpecialRegs sprs;

    union {
        u32 raw;

        struct {
            u32  le :  1; // Little Endian
            u32  ri :  1; // Recoverable interrupt
            u32  pm :  1; // Process marked
            u32     :  1;
            u32  dr :  1; // Data address translation
            u32  ir :  1; // Data address translation
            u32  ip :  1; // Interrupt prefix
            u32     :  1;
            u32 fe1 :  1; // IEEE float exception mode 1
            u32  be :  1; // Branch trace enable
            u32  se :  1; // Single step enable
            u32 fe0 :  1; // IEEE float exception mode 0
            u32  me :  1; // Machine check enable
            u32  fp :  1; // Float available
            u32  pr :  1; // Privilege level
            u32  ee :  1; // External interrupt enable
            u32 ile :  1; // Interrupt Little Endian
            u32     :  1;
            u32 pow :  1; // Power management enable
            u32     : 13;
        };
    } msr;
} Context;

static Context ctx;

static u32 Translate(const u32 addr, const int code) {
    if (!((code && (MSR.ir != 0)) || (!code && (MSR.dr != 0)))) {
        return addr;
    }

    // https://mariokartwii.com/showthread.php?tid=1963

    const Batl* batl = DBATL;
    const Batu* batu = DBATU;

    if (code) {
        batl = IBATL;
        batu = IBATU;
    }

    for (int i = 0; i < NUM_BATS; i++) {
        const u32 length = batu[i].bl << 17;

        const u32 index = ADDR_SEGMENT | (ADDR_PAGE & ~length);

        if ((u32)(batu[i].bepi << 17) == index) {
            // BAT hit
            return (batl[i].brpn << 17) | (ADDR_PAGE & length) | ADDR_OFFSET;
        }
    }

    printf("BAT miss (address: %08X)\n", addr);

    exit(1);
}

MAKEFUNC_BROADWAY_READ(8)
MAKEFUNC_BROADWAY_READ(16)
MAKEFUNC_BROADWAY_READ(32)

static u32 FetchInstr() {
    ctx.cia = ctx.ia;

    const u32 instr = Read32(ctx.ia, NOUWII_TRUE);

    ctx.ia += sizeof(instr);

    return instr;
}

MAKEFUNC_BROADWAY_WRITE(8)
MAKEFUNC_BROADWAY_WRITE(16)
MAKEFUNC_BROADWAY_WRITE(32)

static u32 GetSpr(const u32 spr) {
    switch (spr) {
        case SPR_HID0:
            printf("HID0 read\n");

            return HID0.raw;
        default:
            printf("Unimplemented SPR%u read\n", spr);

            exit(1);
    }
}

static void SetSpr(const u32 spr, const u32 data) {
    if (((spr >= SPR_IBAT0U) && (spr <= SPR_IBAT3L)) ||
        ((spr >= SPR_IBAT4U) && (spr <= SPR_IBAT7L))) {
        u32 idx = (spr - SPR_IBAT0U) / 2;

        if (spr >= SPR_IBAT4U) {
            idx = 4 + (spr - SPR_IBAT4U) / 2;
        }

        if ((spr & 1) != 0) {
            printf("IBAT%uL write (data: %08X)\n", idx, data);

            IBATL[idx].raw = data;
        } else {
            printf("IBAT%uU write (data: %08X)\n", idx, data);

            IBATU[idx].raw = data;
        }

        return;
    }

    if (((spr >= SPR_DBAT0U) && (spr <= SPR_DBAT3L)) ||
        ((spr >= SPR_DBAT4U) && (spr <= SPR_DBAT7L))) {
        u32 idx = (spr - SPR_DBAT0U) / 2;

        if (spr >= SPR_DBAT4U) {
            idx = 4 + (spr - SPR_DBAT4U) / 2;
        }

        if ((spr & 1) != 0) {
            printf("DBAT%uL write (data: %08X)\n", idx, data);

            DBATL[idx].raw = data;
        } else {
            printf("DBAT%uU write (data: %08X)\n", idx, data);

            DBATU[idx].raw = data;
        }

        return;
    }

    switch (spr) {
        case SPR_HID0:
            printf("HID0 write (data: %08X)\n", data);

            HID0.raw = data;

            if (HID0.dce != 0) {
                printf("HID0 D$ enabled\n");
            }

            if (HID0.ice != 0) {
                printf("HID0 I$ enabled\n");
            }

            if (HID0.dcfi != 0) {
                printf("HID0 D$ flash invalidate\n");
            }

            if (HID0.icfi != 0) {
                printf("HID0 I$ flash invalidate\n");
            }

            // Clear flash invalidate bits
            HID0.dcfi = 0;
            HID0.icfi = 0;
            break;
        default:
            printf("Unimplemented SPR%u write (data: %08X)\n", spr, data);

            exit(1);
    }
}

static void ADDI(const u32 instr) {
    u32 n = (i16)UIMM;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addi r%u, r%u, %X; r%u: %08X\n", ctx.cia, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void ADDIS(const u32 instr) {
    u32 n = UIMM << 16;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addis r%u, r%u, %X; r%u: %08X\n", ctx.cia, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void ISYNC(const u32 instr) {
    (void)instr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] isync\n", ctx.cia);
#endif
}

static void LWZ(const u32 instr) {
    u32 addr = (i16)UIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lzw r%u, %d(r%u); r%u: %08X [%08X]\n", ctx.cia, RD, (i16)UIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void MFMSR(const u32 instr) {
    ctx.r[RD] = MSR.raw;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfmsr r%u; r%u: %08X\n", ctx.cia, RD, RD, ctx.r[RD]);
#endif
}

static void MFSPR(const u32 instr) {
    ctx.r[RD] = GetSpr(SPR);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfspr r%u, spr%u; r%u: %08X\n", ctx.cia, RD, SPR, RD, ctx.r[RD]);
#endif
}

static void MTMSR(const u32 instr) {
    MSR.raw = ctx.r[RS];

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtmsr r%u; msr: %08X\n", ctx.cia, RS, ctx.r[RS]);
#endif
}

static void MTSPR(const u32 instr) {
    SetSpr(SPR, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtspr spr%u, r%u; spr%u: %08X\n", ctx.cia, SPR, RS, SPR, ctx.r[RS]);
#endif
}

static void MTSR(const u32 instr) {
    // TODO: Implement SRs
#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtsr sr%u, r%u; sr%u: %08X\n", ctx.cia, RA, RS, RA, ctx.r[RS]);
#endif
}

static void ORI(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | UIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] ori r%u, r%u, %X; r%u: %08X\n", ctx.cia, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void STW(const u32 instr) {
    u32 addr = (i16)UIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stw r%u, %d(r%u); [%08X]: %08X\n", ctx.cia, RS, (i16)UIMM, RA, addr, ctx.r[RS]);
#endif
}

static void ExecInstr(const u32 instr) {
    switch (OPCD) {
        case PRIMARY_ADDI:
            ADDI(instr);
            break;
        case PRIMARY_ADDIS:
            ADDIS(instr);
            break;
        case PRIMARY_SYSTEM:
            switch (XO) {
                case SYSTEM_ISYNC:
                    ISYNC(instr);
                    break;
                default:
                    printf("Unimplemented Broadway system opcode %u (IA: %08X, instruction: %08X)\n", XO, ctx.cia, instr);

                    exit(1);
            }
            break;
        case PRIMARY_ORI:
            ORI(instr);
            break;
        case PRIMARY_REGISTER:
            switch (XO) {
                case SECONDARY_MFMSR:
                    MFMSR(instr);
                    break;
                case SECONDARY_MTMSR:
                    MTMSR(instr);
                    break;
                case SECONDARY_MTSR:
                    MTSR(instr);
                    break;
                case SECONDARY_MFSPR:
                    MFSPR(instr);
                    break;
                case SECONDARY_MTSPR:
                    MTSPR(instr);
                    break;
                default:
                    printf("Unimplemented Broadway secondary opcode %u (IA: %08X, instruction: %08X)\n", XO, ctx.cia, instr);

                    exit(1);
            }
            break;
        case PRIMARY_LWZ:
            LWZ(instr);
            break;
        case PRIMARY_STW:
            STW(instr);
            break;
        default:
            printf("Unimplemented Broadway primary opcode %u (IA: %08X, instruction: %08X)\n", OPCD, ctx.cia, instr);

            exit(1);
    }
}

void broadway_Initialize() {

}

void broadway_Reset() {
    memset(&ctx, 0, sizeof(ctx));

    // HLE boot stub
    ctx.ia = INITIAL_PC;
}

void broadway_Shutdown() {

}

void broadway_Run() {
    for (; ctx.cyclesToRun > 0; ctx.cyclesToRun--) {
        const u32 instr = FetchInstr();

        ExecInstr(instr);
    }
}

i64* broadway_GetCyclesToRun() {
    return &ctx.cyclesToRun;
}
