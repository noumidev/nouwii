/*
 * nouwii is a Nintendo Wii emulator.
 * Copyright (C) 2025  noumidev
 */

#include "hw/broadway.h"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/bit.h"
#include "common/types.h"

#include "core/memory.h"

#include "hw/pi.h"

// #define BROADWAY_DEBUG
// #define BROADWAY_DEBUG_FLOATS

#define NUM_GPRS  (32)
#define NUM_FPRS  (32)
#define NUM_BATS  (8)
#define NUM_GQRS  (8)
#define NUM_CRS   (8)
#define NUM_PMCS  (4)
#define NUM_SPRGS (4)
#define NUM_PS    (2)

#define SIZE_CACHE_BLOCK (0x20)

#define INITIAL_PC (0x3400)

#define MASK_MSR  (0x87C0FF73)
#define MASK_SRR1 (0x783F0000)

enum {
    VECTOR_EXTERNAL_INTERRUPT = 0x500,
    VECTOR_SYSTEM_CALL        = 0xC00,
};

enum {
    PRIMARY_PAIREDSINGLE =  4,
    PRIMARY_MULLI        =  7,
    PRIMARY_SUBFIC       =  8,
    PRIMARY_CMPLI        = 10,
    PRIMARY_CMPI         = 11,
    PRIMARY_ADDIC        = 12,
    PRIMARY_ADDICrc      = 13,
    PRIMARY_ADDI         = 14,
    PRIMARY_ADDIS        = 15,
    PRIMARY_BC           = 16,
    PRIMARY_SC           = 17,
    PRIMARY_B            = 18,
    PRIMARY_SYSTEM       = 19,
    PRIMARY_RLWIMI       = 20,
    PRIMARY_RLWINM       = 21,
    PRIMARY_ORI          = 24,
    PRIMARY_ORIS         = 25,
    PRIMARY_XORI         = 26,
    PRIMARY_XORIS        = 27,
    PRIMARY_ANDIrc       = 28,
    PRIMARY_ANDISrc      = 29,
    PRIMARY_REGISTER     = 31,
    PRIMARY_LWZ          = 32,
    PRIMARY_LWZU         = 33,
    PRIMARY_LBZ          = 34,
    PRIMARY_LBZU         = 35,
    PRIMARY_STW          = 36,
    PRIMARY_STWU         = 37,
    PRIMARY_STB          = 38,
    PRIMARY_STBU         = 39,
    PRIMARY_LHZ          = 40,
    PRIMARY_LHA          = 42,
    PRIMARY_STH          = 44,
    PRIMARY_LMW          = 46,
    PRIMARY_STMW         = 47,
    PRIMARY_LFS          = 48,
    PRIMARY_LFD          = 50,
    PRIMARY_STFS         = 52,
    PRIMARY_STFD         = 54,
    PRIMARY_PSQL         = 56,
    PRIMARY_PSQST        = 60,
    PRIMARY_FLOAT        = 63,
};

enum {
    SECONDARY_CMP    =    0,
    SECONDARY_SUBFC  =    8,
    SECONDARY_ADDC   =   10,
    SECONDARY_MULHWU =   11,
    SECONDARY_MFCR   =   19,
    SECONDARY_LWZX   =   23,
    SECONDARY_SLW    =   24,
    SECONDARY_CNTLZW =   26,
    SECONDARY_AND    =   28,
    SECONDARY_CMPL   =   32,
    SECONDARY_SUBF   =   40,
    SECONDARY_LWZUX  =   55,
    SECONDARY_ANDC   =   60,
    SECONDARY_MULHW  =   75,
    SECONDARY_MFMSR  =   83,
    SECONDARY_DCBF   =   86,
    SECONDARY_LBZX   =   87,
    SECONDARY_NEG    =  104,
    SECONDARY_NOR    =  124,
    SECONDARY_SUBFE  =  136,
    SECONDARY_ADDE   =  138,
    SECONDARY_MTCR   =  144,
    SECONDARY_MTMSR  =  146,
    SECONDARY_STWX   =  151,
    SECONDARY_STWUX  =  183,
    SECONDARY_SUBFZE =  200,
    SECONDARY_ADDZE  =  202,
    SECONDARY_MTSR   =  210,
    SECONDARY_STBX   =  215,
    SECONDARY_MULLW  =  235,
    SECONDARY_ADD    =  266,
    SECONDARY_LHZX   =  279,
    SECONDARY_XOR    =  316,
    SECONDARY_MFSPR  =  339,
    SECONDARY_MFTB   =  371,
    SECONDARY_STHX   =  407,
    SECONDARY_ORC    =  412,
    SECONDARY_OR     =  444,
    SECONDARY_DIVWU  =  459,
    SECONDARY_MTSPR  =  467,
    SECONDARY_DCBI   =  470,
    SECONDARY_DIVW   =  491,
    SECONDARY_SRW    =  536,
    SECONDARY_LSWI   =  597,
    SECONDARY_SYNC   =  598,
    SECONDARY_LFDX   =  599,
    SECONDARY_STSWI  =  725,
    SECONDARY_SRAW   =  792,
    SECONDARY_SRAWI  =  824,
    SECONDARY_EXTSH  =  922,
    SECONDARY_EXTSB  =  954,
    SECONDARY_ICBI   =  982,
    SECONDARY_STFIWX =  983,
    SECONDARY_DCBZ   = 1014,
};

enum {
    SYSTEM_MCRF  =   0,
    SYSTEM_BCLR  =  16,
    SYSTEM_CRNOR =  33,
    SYSTEM_RFI   =  50,
    SYSTEM_ISYNC = 150,
    SYSTEM_CRXOR = 193,
    SYSTEM_CREQV = 289,
    SYSTEM_BCCTR = 528,
};

enum {
    PAIREDSINGLE_PSMR      =  72,
    PAIREDSINGLE_PSMERGE01 = 560,
    PAIREDSINGLE_PSMERGE10 = 592,
};

enum {
    FLOAT_FCMPU  =   0,
    FLOAT_FCTIWZ =  15,
    FLOAT_FDIV   =  18,
    FLOAT_FSUB   =  20,
    FLOAT_FADD   =  21,
    FLOAT_FMUL   =  25,
    FLOAT_FMSUB  =  28,
    FLOAT_FMADD  =  29,
    FLOAT_MTFSB1 =  38,
    FLOAT_FNEG   =  40,
    FLOAT_FMR    =  72,
    FLOAT_MTFSF  = 711,
};

enum {
    SPR_XER    =    1,
    SPR_LR     =    8,
    SPR_CTR    =    9,
    SPR_DAR    =   19,
    SPR_DEC    =   22,
    SPR_SRR0   =   26,
    SPR_SRR1   =   27,
    SPR_TBL    =  268,
    SPR_TBU    =  269,
    SPR_SPRG0  =  272,
    SPR_SPRG3  =  275,
    SPR_IBAT0U =  528,
    SPR_IBAT3L =  535,
    SPR_DBAT0U =  536,
    SPR_DBAT3L =  543,
    SPR_IBAT4U =  560,
    SPR_IBAT7L =  567,
    SPR_DBAT4U =  568,
    SPR_DBAT7L =  575,
    SPR_GQR0   =  912,
    SPR_GQR7   =  919,
    SPR_HID2   =  920,
    SPR_MMCR0  =  952,
    SPR_PMC1   =  953,
    SPR_PMC2   =  954,
    SPR_MMCR1  =  956,
    SPR_PMC3   =  957,
    SPR_PMC4   =  958,
    SPR_HID0   = 1008,
    SPR_HID4   = 1011,
    SPR_L2CR   = 1017,
};

enum {
    COND_SO = 0,
    COND_UN = 0,
    COND_EQ = 1,
    COND_GT = 2,
    COND_LT = 3,
};

enum {
    QUANT_TYPE_FLOAT = 0,
};

#define MAKEFUNC_BROADWAY_READ(size)                        \
static u##size Read##size(const u32 addr, const int code) { \
    return memory_Read##size(Translate(addr, code));        \
}                                                           \

#define MAKEFUNC_BROADWAY_WRITE(size)                         \
static void Write##size(const u32 addr, const u##size data) { \
    memory_Write##size(Translate(addr, NOUWII_FALSE), data);  \
}                                                             \

#define TO_IBM_POS(n) (31 - n)

static u32 GetMask(const u32 start, const u32 end) {
    if (start <= end) {
        return (0xFFFFFFFFU << TO_IBM_POS(end)) & (0xFFFFFFFFU >> start);
    }

    return (0xFFFFFFFFU << TO_IBM_POS(end)) | (0xFFFFFFFFU >> start); 
}

static u32 GetBits(const u32 n, const u32 start, const u32 end) {
    return (n & GetMask(start, end)) >> TO_IBM_POS(end);
}

static u32 SetBits(const u32 n, const u32 start, const u32 end, const u32 data) {
    const u32 mask = GetMask(start, end);

    return (n & ~mask) | ((data << TO_IBM_POS(end)) & mask);
}

// Instruction fields
#define OPCD (GetBits(instr,  0,  5))
#define   XO (GetBits(instr, 21, 30))
#define  FXO (GetBits(instr, 26, 30))
#define   FC (GetBits(instr, 21, 25))
#define   RA (GetBits(instr, 11, 15))
#define   RB (GetBits(instr, 16, 20))
#define   RD (GetBits(instr,  6, 10))
#define   RS (GetBits(instr,  6, 10))
#define CRFD (GetBits(instr,  6,  8))
#define CRFS (GetBits(instr, 11, 13))
#define   SH (GetBits(instr, 16, 20))
#define   MB (GetBits(instr, 21, 25))
#define   ME (GetBits(instr, 26, 30))
#define   FM (GetBits(instr,  7, 14))
#define   BO (GetBits(instr,  6, 10))
#define   BI (GetBits(instr, 11, 15))
#define   BD (GetBits(instr, 16, 29))
#define   LI (GetBits(instr,  6, 29))
#define    D (GetBits(instr, 20, 31))
#define    I (GetBits(instr, 17, 19))
#define    W (GetBits(instr, 16, 16) != 0)
#define    L (GetBits(instr, 10, 10) != 0)
#define   AA (GetBits(instr, 30, 30) != 0)
#define   RC (GetBits(instr, 31, 31) != 0)
#define   LK (GetBits(instr, 31, 31) != 0)
#define UIMM (GetBits(instr, 16, 31))
#define SIMM ((i16)GetBits(instr, 16, 31))
#define  SPR (GetBits(instr, 11, 15) | (GetBits(instr, 16, 20) << 5))

// Branch control fields
#define BO_TEST_COND (GetBits(instr, 6, 6) == 0)
#define BO_COND_TRUE (GetBits(instr, 7, 7) != 0)
#define BO_TEST_CTR  (GetBits(instr, 8, 8) == 0)
#define BO_CTR_ZERO  (GetBits(instr, 9, 9) != 0)

// Address fields
#define ADDR_OFFSET  (addr & 0x0001FFFF)
#define ADDR_PAGE    (addr & 0x0FFE0000)
#define ADDR_SEGMENT (addr & 0xF0000000)

// SPRs
#define   XER (ctx.sprs.xer)
#define   CTR (ctx.sprs.ctr)
#define    LR (ctx.sprs.lr)
#define   DAR (ctx.sprs.dar)
#define   DEC (ctx.sprs.dec)
#define   TBR (ctx.sprs.tbr.raw)
#define   TBL (ctx.sprs.tbr.tbl)
#define   TBU (ctx.sprs.tbr.tbu)
#define  HID0 (ctx.sprs.hid0)
#define  HID2 (ctx.sprs.hid2)
#define  HID4 (ctx.sprs.hid4)
#define MMCR0 (ctx.sprs.mmcr0)
#define MMCR1 (ctx.sprs.mmcr1)
#define   PMC (ctx.sprs.pmc)
#define DBATL (ctx.sprs.dbatl)
#define DBATU (ctx.sprs.dbatu)
#define IBATL (ctx.sprs.ibatl)
#define IBATU (ctx.sprs.ibatu)
#define   GQR (ctx.sprs.gqr)
#define  L2CR (ctx.sprs.l2cr)
#define  SPRG (ctx.sprs.sprg)
#define  SRR0 (ctx.sprs.srr0)
#define  SRR1 (ctx.sprs.srr1)

#define    IA (ctx.ia)
#define   CIA (ctx.cia)
#define    CR (ctx.cr)
#define FPSCR (ctx.fpscr)
#define   MSR (ctx.msr)

#define PS0 ps[0]
#define PS1 ps[1]

typedef union Msr {
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
} Msr;

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

typedef union Counter {
    u32 raw;

    struct {
        u32 counter : 31;
        u32      ov :  1;
    };
} Counter;

typedef struct SpecialRegs {
    union {
        u32 raw;

        struct {
            u32 bytecount :  7;
            u32           : 22;
            u32        ca :  1;
            u32        ov :  1;
            u32        so :  1;
        };
    } xer;

    union {
        u64 raw;

        struct {
            u64 tbl : 32;
            u64 tbu : 32;
        };
    } tbr;

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

    union {
        u32 raw;

        struct {
            u32        : 16;
            u32  dqoee :  1; // DMA queue overflow error enable
            u32  dcmee :  1; // DMA cache miss error enable
            u32  dncee :  1; // DMA normal cache access error enable
            u32  dchee :  1; // dcbz_l cache hit error enable
            u32 dqoerr :  1; // DMA queue overflow error
            u32 dcmerr :  1; // DMA cache miss error
            u32 dncerr :  1; // DMA normal cache access error
            u32 dcherr :  1; // dcbz_l cache hit error
            u32  dmaql :  4; // DMA queue length
            u32    lce :  1; // Locked cache enable
            u32    pse :  1; // Paired single enable
            u32    wpe :  1; // Write pipe enable
            u32   lsqe :  1; // Quantized load/store enable
        };
    } hid2;

    union {
        u32 raw;

        struct {
            u32       : 20;
            u32 l2cfi :  1;
            u32 l2mum :  1;
            u32   dbp :  1;
            u32   lpe :  1;
            u32   st0 :  1;
            u32   sbe :  1;
            u32       :  1;
            u32   bpd :  2;
            u32  l2fm :  2;
            u32       :  1;
        };
    } hid4;

    union {
        u32 raw;

        struct {
            u32  sttype : 3;
            u32         : 5;
            u32 stscale : 6;
            u32         : 2;
            u32  ldtype : 3;
            u32         : 5;
            u32 ldscale : 6;
            u32         : 2;
        };
    } gqr[NUM_GQRS];

    union {
        u32 raw;

        struct {
            u32 l2ip :  1; // L2 global invalidate in progress
            u32      : 17;
            u32 l2ts :  1; // L2 test support
            u32 l2wt :  1; // L2 write-through
            u32      :  1;
            u32  l2i :  1; // L2 global invalidate
            u32      :  6;
            u32 l2ce :  1; // L2 checkstop enable
            u32  l2e :  1; // L2 enable
        };
    } l2cr;

    union {
        u32 raw;

        struct {
            u32     pmc2select : 6; // PMC2 input select
            u32     pmc1select : 7; // PMC1 input select
            u32     pmctrigger : 1; // Trigger PMC2-4
            u32  pmcintcontrol : 1; // Enable PMC2-4 interrupts
            u32 pmc1intcontrol : 1; // Enable PMC1 interrupts
            u32      threshold : 6;
            u32  intonbittrans : 1; // Interrupt on transition
            u32      rtcselect : 2; // TBL selection enable
            u32       discount : 1; // Disable counting after interrupt
            u32          enint : 1; // Enable interrupt
            u32            dmr : 1; // Disable counting while MSR[PM] = 0
            u32            dms : 1; // Disable counting while MSR[PM] = 1
            u32             du : 1; // Disable counting in User mode
            u32             dp : 1; // Disable counting in Supervisor mode
            u32            dis : 1; // Disable counting
        };
    } mmcr0;

    union {
        u32 raw;

        struct {
            u32            : 22;
            u32 pmc4select :  5;
            u32 pmc3select :  5;
        };
    } mmcr1;

    Counter pmc[NUM_PMCS];
    Counter dec;

    Batl dbatl[NUM_BATS], ibatl[NUM_BATS];
    Batu dbatu[NUM_BATS], ibatu[NUM_BATS];

    u32 srr0;
    Msr srr1;

    u32 sprg[NUM_SPRGS];

    u32 dar;

    u32 lr;
    u32 ctr;
} SpecialRegs;

typedef struct Context {
    i64 cyclesToRun;

    u32 ia, cia;

    u32 r[NUM_GPRS];

    union {
        u64 raw[NUM_PS];
        f64 ps[NUM_PS];
    } fprs[NUM_FPRS];

    u32 cr, fpscr;

    SpecialRegs sprs;

    Msr msr;
} Context;

static Context ctx;

static void SaveExceptionContext() {
    // Save IA and MSR
    SRR0 = IA;
    SRR1.raw &= ~(MASK_SRR1 | MASK_MSR);
    SRR1.raw |= MSR.raw & MASK_MSR;

    MSR.le  = MSR.ile;
    MSR.ri  = 0;
    MSR.dr  = 0;
    MSR.ir  = 0;
    MSR.fe1 = 0;
    MSR.be  = 0;
    MSR.se  = 0;
    MSR.fe0 = 0;
    MSR.fp  = 0;
    MSR.pr  = 0;
    MSR.ee  = 0;
    MSR.pow = 0;
}

static void ExternalInterrupt() {
    printf("Broadway External interrupt exception (CIA: %08X)\n", CIA);

    SaveExceptionContext();

    IA = VECTOR_EXTERNAL_INTERRUPT;
}

static void SystemCall() {
    printf("Broadway System call exception (CIA: %08X)\n", CIA);

    SaveExceptionContext();

    IA = VECTOR_SYSTEM_CALL;
}

static void CheckInterrupt() {
    if (pi_IsIrqAsserted() && (MSR.ee != 0)) {
        ExternalInterrupt();
    }
}

static void SetCr(const int cr, const u32 n) {
    const int index = 4 * cr;

    CR = SetBits(CR, index, index + 3, n);
}

static void SetFpscr(const int cr, const u32 n) {
    const int index = 4 * cr;

    FPSCR = SetBits(FPSCR, index, index + 3, n);
}

static void SetFlags(const int cr, const u32 n) {
    const u32 so = XER.so;
    const u32 eq = n == 0;
    const u32 gt = (i32)n > 0;
    const u32 lt = (i32)n < 0;

    SetCr(cr, (lt << COND_LT) | (gt << COND_GT) | (eq << COND_EQ) | so);
}

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

    // Number of enabled BATs depends on HID4
    for (int i = 0; i < (4 + 4 * HID4.sbe); i++) {
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
MAKEFUNC_BROADWAY_READ(64)

static u32 FetchInstr() {
    CIA = IA;

    const u32 instr = Read32(IA, NOUWII_TRUE);

    IA += sizeof(instr);

    return instr;
}

MAKEFUNC_BROADWAY_WRITE(8)
MAKEFUNC_BROADWAY_WRITE(16)
MAKEFUNC_BROADWAY_WRITE(32)
MAKEFUNC_BROADWAY_WRITE(64)

static u32 GetSpr(const u32 spr) {
    if ((spr >= SPR_SPRG0) && (spr <= SPR_SPRG3)) {
        const u32 idx = spr - SPR_SPRG0;

        printf("SPRG%u read\n", idx);

        return SPRG[idx];
    }

    if ((spr >= SPR_GQR0) && (spr <= SPR_GQR7)) {
        const u32 idx = spr - SPR_GQR0;

        printf("GQR%u read\n", idx);

        return GQR[idx].raw;
    }

    switch (spr) {
        case SPR_XER:
            printf("XER read\n");

            return XER.raw;
        case SPR_LR:
            printf("LR read\n");

            return LR;
        case SPR_CTR:
            printf("CTR read\n");

            return CTR;
        case SPR_DAR:
            printf("DAR read\n");

            return DAR;
        case SPR_DEC:
            printf("DEC read\n");

            return DEC.raw;
        case SPR_SRR0:
            printf("SRR0 read\n");

            return SRR0;
        case SPR_SRR1:
            printf("SRR1 read\n");

            return SRR1.raw;
        case SPR_TBL:
            printf("TBL read\n");

            return TBL;
        case SPR_TBU:
            printf("TBU read\n");

            return TBU;
        case SPR_HID2:
            printf("HID2 read\n");

            return HID2.raw;
        case SPR_MMCR0:
            printf("MMCR0 read\n");

            return MMCR0.raw;
        case SPR_PMC1:
            printf("PMC1 read\n");

            return PMC[0].raw;
        case SPR_PMC2:
            printf("PMC2 read\n");

            return PMC[1].raw;
        case SPR_MMCR1:
            printf("MMCR1 read\n");

            return MMCR1.raw;
        case SPR_PMC3:
            printf("PMC3 read\n");

            return PMC[2].raw;
        case SPR_PMC4:
            printf("PMC4 read\n");

            return PMC[3].raw;
        case SPR_HID0:
            printf("HID0 read\n");

            return HID0.raw;
        case SPR_HID4:
            printf("HID4 read\n");

            // Bit 31 is always set
            return HID4.raw | (1U << 31);
        case SPR_L2CR:
            printf("L2CR read\n");

            return L2CR.raw;
        default:
            printf("Unimplemented SPR%u read\n", spr);

            exit(1);
    }
}

static void SetSpr(const u32 spr, const u32 data) {
    if ((spr >= SPR_SPRG0) && (spr <= SPR_SPRG3)) {
        const u32 idx = spr - SPR_SPRG0;

        printf("SPRG%u write (data: %08X)\n", idx, data);

        SPRG[idx] = data;

        return;
    }

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

    if ((spr >= SPR_GQR0) && (spr <= SPR_GQR7)) {
        const u32 idx = spr - SPR_GQR0;

        printf("GQR%u write (data: %08X)\n", idx, data);

        GQR[idx].raw = data;

        return;
    }

    switch (spr) {
        case SPR_XER:
            printf("XER write (data: %08X)\n", data);

            XER.raw = data;
            break;
        case SPR_LR:
            printf("LR write (data: %08X)\n", data);

            LR = data;
            break;
        case SPR_CTR:
            // printf("CTR write (data: %08X)\n", data);

            CTR = data;
            break;
        case SPR_DAR:
            printf("DAR write (data: %08X)\n", data);

            DAR = data;
            break;
        case SPR_DEC:
            printf("DEC write (data: %08X)\n", data);
            
            assert(data == 0);

            DEC.raw = data;
            break;
        case SPR_SRR0:
            printf("SRR0 write (data: %08X)\n", data);

            SRR0 = data;
            break;
        case SPR_SRR1:
            printf("SRR1 write (data: %08X)\n", data);

            SRR1.raw = data;
            break;
        case SPR_HID2:
            printf("HID2 write (data: %08X)\n", data);

            HID2.raw = data;

            if (HID2.pse != 0) {
                printf("HID2 Paired Singles enabled\n");
            }

            if (HID2.wpe != 0) {
                printf("HID2 Write-gather pipe enabled\n");
            }

            if (HID2.lsqe != 0) {
                printf("HID2 Quantized loadstores enabled\n");
            }
            break;
        case SPR_MMCR0:
            printf("MMCR0 write (data: %08X)\n", data);

            MMCR0.raw = data;
            break;
        case SPR_PMC1:
            printf("PMC1 write (data: %08X)\n", data);

            PMC[0].raw = data;
            break;
        case SPR_PMC2:
            printf("PMC2 write (data: %08X)\n", data);

            PMC[1].raw = data;
            break;
        case SPR_MMCR1:
            printf("MMCR1 write (data: %08X)\n", data);

            MMCR1.raw = data;
            break;
        case SPR_PMC3:
            printf("PMC3 write (data: %08X)\n", data);

            PMC[2].raw = data;
            break;
        case SPR_PMC4:
            printf("PMC4 write (data: %08X)\n", data);

            PMC[3].raw = data;
            break;
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
        case SPR_HID4:
            printf("HID4 write (data: %08X)\n", data);

            HID4.raw = data;

            if (HID4.sbe != 0) {
                printf("HID4 secondary BATs enabled\n");
            }
            break;
        case SPR_L2CR:
            printf("L2CR write (data: %08X)\n", data);

            L2CR.raw = data;

            if (L2CR.l2i != 0) {
                printf("L2CR global invalidate\n");

                // Simulate L2 invalidation
                L2CR.l2ip = 0;
            }

            if (L2CR.l2e != 0) {
                printf("L2CR L2 enabled\n");
            }
            break;
        default:
            printf("Unimplemented SPR%u write (data: %08X)\n", spr, data);

            exit(1);
    }
}

static void ADD(const u32 instr) {
    ctx.r[RD] = ctx.r[RA] + ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] add%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void ADDC(const u32 instr) {
    const u64 n = (u64)ctx.r[RA] + (u64)ctx.r[RB];

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addc%s r%u, r%u, r%u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD], XER.raw);
#endif
}

static void ADDE(const u32 instr) {
    const u64 n = (u64)ctx.r[RA] + (u64)ctx.r[RB] + (u64)XER.ca;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] adde%s r%u, r%u, r%u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD], XER.raw);
#endif
}

static void ADDI(const u32 instr) {
    u32 n = SIMM;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addi r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void ADDIC(const u32 instr) {
    const u64 n = (u64)ctx.r[RA] + (u64)SIMM;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = ctx.r[RA] + (u64)SIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addic r%u, r%u, %X; r%u: %08X, xer: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD], XER.raw);
#endif
}

static void ADDICrc(const u32 instr) {
    const u64 n = (u64)ctx.r[RA] + (u64)SIMM;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    SetFlags(0, ctx.r[RD]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addic. r%u, r%u, %X; r%u: %08X, xer: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD], XER.raw);
#endif
}

static void ADDIS(const u32 instr) {
    u32 n = UIMM << 16;

    if (RA != 0) {
        n += ctx.r[RA];
    }

    ctx.r[RD] = n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addis r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD]);
#endif
}

static void ADDZE(const u32 instr) {
    const u64 n = (u64)ctx.r[RA] + (u64)XER.ca;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] addze%s r%u, r%u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RD, RA, RD, ctx.r[RD], XER.raw);
#endif
}

static void AND(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] & ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] and%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void ANDC(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] & ~ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] andc%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void ANDIrc(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] & UIMM;

    SetFlags(0, ctx.r[RA]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] andi. r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void ANDISrc(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] & (UIMM << 16);

    SetFlags(0, ctx.r[RA]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] andis. r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void B(const u32 instr) {
    u32 target = (i32)(LI << 8) >> 6;

    if (!AA) {
        target += CIA;
    }

    IA = target;

    if (LK) {
        LR = CIA + sizeof(instr);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bc%s%s %08X; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", (AA) ? "a" : "", target, IA, LR);
#endif
}

static void BC(const u32 instr) {
    if (BO_TEST_CTR) {
        CTR--;
    }

    const int ctrOk = !BO_TEST_CTR || ((CTR != 0) != BO_CTR_ZERO);
    const int condOk = !BO_TEST_COND || (GetBits(CR, BI, BI) == BO_COND_TRUE);

    u32 target = (i16)(BD << 2);

    if (!AA) {
        target += CIA;
    }

    if (ctrOk && condOk) {
        IA = target;

        if (LK) {
            LR = CIA + sizeof(instr);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bc%s%s %u, %u, %08X; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", (AA) ? "a" : "", BO, BI, target, IA, LR);
#endif
}

static void BCCTR(const u32 instr) {
    assert(!BO_TEST_CTR);

    const int condOk = !BO_TEST_COND || (GetBits(CR, BI, BI) == BO_COND_TRUE);

    if (condOk) {
        IA = CTR & ~3;

        if (LK) {
            LR = CIA + sizeof(instr);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bcctr%s %u, %u; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", BO, BI, IA, LR);
#endif
}

static void BCLR(const u32 instr) {
    if (BO_TEST_CTR) {
        CTR--;
    }

    const int ctrOk = !BO_TEST_CTR || ((CTR != 0) != BO_CTR_ZERO);
    const int condOk = !BO_TEST_COND || (GetBits(CR, BI, BI) == BO_COND_TRUE);

    if (ctrOk && condOk) {
        IA = LR;

        if (LK) {
            LR = CIA + sizeof(instr);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] bclr%s %u, %u; nia: %08X, lr: %08X\n", CIA, (LK) ? "l" : "", BO, BI, IA, LR);
#endif
}

void CMP(const u32 instr) {
    assert(!L);

    const i32 a = (i32)ctx.r[RA];
    const i32 b = (i32)ctx.r[RB];

    u32 n = XER.so;

    if (a < b) {
        n |= 1 << COND_LT;
    } else if (a > b) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmp crf%u, %d, r%u, r%u; cr: %08X\n", CIA, CRFD, L, RA, RB, CR);
#endif
}

void CMPI(const u32 instr) {
    assert(!L);

    const i32 a = (i32)ctx.r[RA];

    u32 n = XER.so;

    if (a < SIMM) {
        n |= 1 << COND_LT;
    } else if (a > SIMM) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmpi crf%u, %d, r%u, %X; cr: %08X\n", CIA, CRFD, L, RA, SIMM, CR);
#endif
}

void CMPL(const u32 instr) {
    assert(!L);

    const u32 a = ctx.r[RA];
    const u32 b = ctx.r[RB];

    u32 n = XER.so;

    if (a < b) {
        n |= 1 << COND_LT;
    } else if (a > b) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmpl crf%u, %d, r%u, r%u; cr: %08X\n", CIA, CRFD, L, RA, RB, CR);
#endif
}

void CMPLI(const u32 instr) {
    assert(!L);

    const u32 a = ctx.r[RA];

    u32 n = XER.so;

    if (a < UIMM) {
        n |= 1 << COND_LT;
    } else if (a > UIMM) {
        n |= 1 << COND_GT;
    } else {
        n |= 1 << COND_EQ;
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cmpli crf%u, %d, r%u, %X; cr: %08X\n", CIA, CRFD, L, RA, UIMM, CR);
#endif
}

static void CNTLZW(const u32 instr) {
    ctx.r[RA] = common_Clz(ctx.r[RS]);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] cntlzw%s r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RA, ctx.r[RA]);
#endif
}

static void CREQV(const u32 instr) {
    CR = SetBits(CR, RD, RD, ~(GetBits(CR, RA, RA) ^ GetBits(CR, RB, RB)));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] creqv crb%u, crb%u, crb%u; cr: %08X\n", CIA, RD, RA, RB, CR);
#endif
}

static void CRNOR(const u32 instr) {
    CR = SetBits(CR, RD, RD, ~(GetBits(CR, RA, RA) | GetBits(CR, RB, RB)));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] crnor crb%u, crb%u, crb%u; cr: %08X\n", CIA, RD, RA, RB, CR);
#endif
}

static void CRXOR(const u32 instr) {
    CR = SetBits(CR, RD, RD, GetBits(CR, RA, RA) ^ GetBits(CR, RB, RB));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] crxor crb%u, crb%u, crb%u; cr: %08X\n", CIA, RD, RA, RB, CR);
#endif
}

static void DCBF(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement data cache?

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] dcbf r%u, r%u; flush block @ [%08X]\n", CIA, RA, RB, addr);
#endif
}

static void DCBI(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement data cache?

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] dcbf r%u, r%u; invalidate block @ [%08X]\n", CIA, RA, RB, addr);
#endif
}

static void DCBZ(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // HACK
    for (u32 i = 0; i < SIZE_CACHE_BLOCK; i += sizeof(u64)) {
        Write64(addr + i, 0);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] dcbz r%u, r%u; clear block @ [%08X]\n", CIA, RA, RB, addr);
#endif
}

static void DIVW(const u32 instr) {
    const i32 n = (i32)ctx.r[RA];
    const i32 d = (i32)ctx.r[RB];

    assert(d != 0);
    assert(!((n == INT32_MIN) && (d == -1)));

    ctx.r[RD] = (u32)(n / d);

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] divw%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void DIVWU(const u32 instr) {
    const u32 n = ctx.r[RA];
    const u32 d = ctx.r[RB];

    assert(d != 0);

    ctx.r[RD] = n / d;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] divwu%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void EXTSB(const u32 instr) {
    ctx.r[RA] = (i8)ctx.r[RS];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] extsb%s r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RA, ctx.r[RA]);
#endif
}

static void EXTSH(const u32 instr) {
    ctx.r[RA] = (i16)ctx.r[RS];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] extsh%s r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RA, ctx.r[RA]);
#endif
}

static void FADD(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0 + ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fadd%s f%u, f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0);
#endif
}

static void FCMPU(const u32 instr) {
    const f64 a = ctx.fprs[RA].PS0;
    const f64 b = ctx.fprs[RB].PS0;

    u32 n;

    if (isnan(a) || isnan(b)) {
        n = COND_UN;
    } else {
        if (a < b) {
            n = 1 << COND_LT;
        } else if (a > b) {
            n = 1 << COND_GT;
        } else {
            n = 1 << COND_EQ;
        }
    }

    SetCr(CRFD, n);

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fcmpu crf%u, f%u, f%u; cr: %08X\n", CIA, CRFD, RA, RB, CR);
#endif
}

static void FCTIWZ(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].raw[0] = (u32)floor(ctx.fprs[RB].PS0);

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fctiwz%s f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RB, ctx.fprs[RD].raw[0]);
#endif
}

static void FDIV(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0 / ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fdiv%s f%u, f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0);
#endif
}

static void FMADD(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0 * ctx.fprs[FC].PS0 + ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fmadd%s f%u, f%u, f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RA, FC, RB, ctx.fprs[RD].PS0);
#endif
}

static void FMR(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RB].PS0;

    if (HID2.pse == 0) {
        ctx.fprs[RD].PS1 = ctx.fprs[RB].PS0;
    }

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fmr%s f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void FMSUB(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0 * ctx.fprs[FC].PS0 - ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fmsub%s f%u, f%u, f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RA, FC, RB, ctx.fprs[RD].PS0);
#endif
}

static void FMUL(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0 * ctx.fprs[FC].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fmul%s f%u, f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RA, FC, ctx.fprs[RD].PS0);
#endif
}

static void FNEG(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = -ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fneg%s f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RB, ctx.fprs[RD].PS0);
#endif
}

static void FSUB(const u32 instr) {
    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0 - ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] fsub%s f%u, f%u, f%u; ps0: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0);
#endif
}

static void ICBI(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement instruction cache?

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] icbi r%u, r%u; invalidate block @ [%08X]\n", CIA, RA, RB, addr);
#endif
}

static void ISYNC(const u32 instr) {
    (void)instr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] isync\n", CIA);
#endif
}

static void LBZ(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = (u32)Read8(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lbz r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LBZU(const u32 instr) {
    assert(RA != 0);
    assert(RA != RD);

    const u32 addr = ctx.r[RA] + SIMM;

    ctx.r[RD] = (u8)Read8(addr, NOUWII_FALSE);
    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lbzu r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LBZX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = (u32)Read8(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lhzx r%u, r%u, r%u; r%u: %08X [%08X]\n", CIA, RD, RA, RB, RD, ctx.r[RD], addr);
#endif
}

static void LFD(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.fprs[RD].PS0 = common_ToF64(Read64(addr, NOUWII_FALSE));

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] lfd f%u, %d(r%u); f%u: %lf [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.fprs[RD].PS0, addr);
#endif
}

static void LFDX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.fprs[RD].PS0 = common_ToF64(Read64(addr, NOUWII_FALSE));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lfdx r%u, r%u, r%u; f%u: %lf [%08X]\n", CIA, RD, RA, RB, RD, ctx.fprs[RD].PS0, addr);
#endif
}

static void LFS(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    const f32 data = common_ToF32(Read32(addr, NOUWII_FALSE));

    ctx.fprs[RD].PS0 = (f64)data;

    if (HID2.pse != 0) {
        ctx.fprs[RD].PS1 = (f64)data;
    }

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] lfs f%u, %d(r%u); f%u: %lf [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.fprs[RD].PS0, addr);
#endif
}

static void LHA(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = (i16)Read16(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lha r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LHZ(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = (u32)Read16(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lhz r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LHZX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = (u32)Read16(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lhzx r%u, r%u, r%u; r%u: %08X [%08X]\n", CIA, RD, RA, RB, RD, ctx.r[RD], addr);
#endif
}

static void LMW(const u32 instr) {
    assert(RA < RD);

    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    for (int r = RD; r < NUM_GPRS; r++) {
        ctx.r[r] = Read32(addr, NOUWII_FALSE);

        addr += sizeof(u32);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lmw r%u, %d(r%u)\n", CIA, RS, SIMM, RA);
#endif
}

static void LSWI(const u32 instr) {
    u32 addr = 0;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    int n = RB;

    // NB = 0 encodes 32
    if (n == 0) {
        n = 32;
    }

    u32 r = RD;
    u32 i = 0;

    for (; n > 0; n--) {
        ctx.r[r] = SetBits(ctx.r[r], i, i + 7, (u32)Read8(addr++, NOUWII_FALSE));

        i = (i + 8) & 31;

        if (i == 0) {
            r = (r + 1) & (NUM_GPRS - 1);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lswi r%u, r%u, %u\n", CIA, RD, RA, RB);
#endif
}

static void LWZ(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lwz r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LWZU(const u32 instr) {
    assert(RA != 0);
    assert(RA != RD);

    const u32 addr = ctx.r[RA] + SIMM;

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);
    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lwzu r%u, %d(r%u); r%u: %08X [%08X]\n", CIA, RD, SIMM, RA, RD, ctx.r[RD], addr);
#endif
}

static void LWZUX(const u32 instr) {
    assert(RA != 0);
    assert(RA != RD);

    const u32 addr = ctx.r[RA] + ctx.r[RB];

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);
    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lwzux r%u, r%u, r%u; r%u: %08X [%08X]\n", CIA, RD, RA, RB, RD, ctx.r[RD], addr);
#endif
}

static void LWZX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    ctx.r[RD] = Read32(addr, NOUWII_FALSE);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] lwzx r%u, r%u, r%u; r%u: %08X [%08X]\n", CIA, RD, RA, RB, RD, ctx.r[RD], addr);
#endif
}

static void MCRF(const u32 instr) {
    CR = SetBits(CR, 4 * CRFD, 4 * CRFD + 3, GetBits(CR, 4 * CRFS, 4 * CRFS + 3));

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mcrf crf%u, crf%u; cr: %08X\n", CIA, CRFD, CRFS, CR);
#endif
}

static void MFCR(const u32 instr) {
    ctx.r[RD] = CR;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfcr r%u; r%u: %08X\n", CIA, RD, RD, ctx.r[RD]);
#endif
}

static void MFMSR(const u32 instr) {
    ctx.r[RD] = MSR.raw;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfmsr r%u; r%u: %08X\n", CIA, RD, RD, ctx.r[RD]);
#endif
}

static void MFSPR(const u32 instr) {
    ctx.r[RD] = GetSpr(SPR);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mfspr r%u, spr%u; r%u: %08X\n", CIA, RD, SPR, RD, ctx.r[RD]);
#endif
}

static void MFTB(const u32 instr) {
    ctx.r[RD] = GetSpr(SPR);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mftb r%u, spr%u; r%u: %08X\n", CIA, RD, SPR, RD, ctx.r[RD]);
#endif
}

static void MTCR(const u32 instr) {
    CR = ctx.r[RS];

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtcr r%u; cr: %08X\n", CIA, RS, ctx.r[RS]);
#endif
}

static void MTFSB1(const u32 instr) {
    assert(!RC);

    FPSCR = SetBits(FPSCR, RD, RD, 1);

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] mtfsb1 crb%u; fpscr: %08X\n", CIA, RD, FPSCR);
#endif
}

static void MTFSF(const u32 instr) {
    assert(!RC);

    const u32 n = (u32)common_FromF64(ctx.fprs[RB].PS0);

    for (int i = 0; i < NUM_CRS; i++) {
        if ((FM & (1 << (NUM_CRS - i - 1))) != 0) {
            SetFpscr(i, n);
        }
    }

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] mtfsf %02X, f%u; fpscr: %08X\n", CIA, FM, RB, FPSCR);
#endif
}

static void MTMSR(const u32 instr) {
    MSR.raw = ctx.r[RS];

    CheckInterrupt();

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtmsr r%u; msr: %08X\n", CIA, RS, ctx.r[RS]);
#endif
}

static void MTSPR(const u32 instr) {
    SetSpr(SPR, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtspr spr%u, r%u; spr%u: %08X\n", CIA, SPR, RS, SPR, ctx.r[RS]);
#endif
}

static void MTSR(const u32 instr) {
    (void)instr;

    // TODO: Implement SRs
#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mtsr sr%u, r%u; sr%u: %08X\n", CIA, RA, RS, RA, ctx.r[RS]);
#endif
}

static void MULHW(const u32 instr) {
    ctx.r[RD] = (u32)(((i64)ctx.r[RA] * (i64)ctx.r[RB]) >> 32);

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mulhw%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void MULHWU(const u32 instr) {
    ctx.r[RD] = (u32)(((u64)ctx.r[RA] * (u64)ctx.r[RB]) >> 32);

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mulhwu%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void MULLI(const u32 instr) {
    ctx.r[RD] = ctx.r[RA] * SIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mulli r%u, r%u, %X; r%u: %08X\n", CIA, RD, RA, SIMM, RD, ctx.r[RD]);
#endif
}

static void MULLW(const u32 instr) {
    ctx.r[RD] = ctx.r[RA] * ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] mullw%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void NEG(const u32 instr) {
    ctx.r[RD] = -ctx.r[RA];

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] neg%s r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RD, ctx.r[RD]);
#endif
}

static void NOR(const u32 instr) {
    ctx.r[RA] = ~(ctx.r[RS] | ctx.r[RB]);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] nor%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void OR(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] or%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void ORC(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | ~ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] orc%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void ORI(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | UIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] ori r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void ORIS(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] | (UIMM << 16);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] oris r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void PSMERGE01(const u32 instr) {
    assert(HID2.pse != 0);

    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS0;
    ctx.fprs[RD].PS1 = ctx.fprs[RB].PS1;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] ps_merge01%s f%u, f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void PSMERGE10(const u32 instr) {
    assert(HID2.pse != 0);

    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD].PS0 = ctx.fprs[RA].PS1;
    ctx.fprs[RD].PS1 = ctx.fprs[RB].PS0;

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] ps_merge10%s f%u, f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RA, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void PSMR(const u32 instr) {
    assert(HID2.pse != 0);

    // TODO: Implement flags
    assert(!RC);

    ctx.fprs[RD] = ctx.fprs[RB];

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] ps_mr%s f%u, f%u; ps0: %lf, ps1: %lf\n", CIA, (RC) ? "." : "", RD, RB, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1);
#endif
}

static void PSQL(const u32 instr) {
    assert((HID2.pse != 0) && (HID2.lsqe != 0));

    u32 addr = (i32)(D << 20) >> 20;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement scaling
    assert(GQR[I].ldscale == 0);

    switch (GQR[I].ldtype) {
        case QUANT_TYPE_FLOAT:
            ctx.fprs[RD].PS0 = (f64)common_ToF32(Read32(addr, NOUWII_FALSE));
            break;
        default:
            printf("Unimplemented psq_l dequantization type %u\n", GQR[I].ldtype);
            exit(1);
    }

    if (W) {
        ctx.fprs[RD].PS1 = 1.0;
    } else {
        switch (GQR[I].ldtype) {
            case QUANT_TYPE_FLOAT:
                ctx.fprs[RD].PS1 = (f64)common_ToF32(Read32(addr + sizeof(f32), NOUWII_FALSE));
                break;
            default:
                printf("Unimplemented psq_l dequantization type %u\n", GQR[I].ldtype);
                exit(1);
        }
    }

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] psq_l f%u, %d(r%u), %d, %u; ps0: %lf, ps1: %lf [%08X]\n", CIA, RD, (i32)(D << 20) >> 20, RA, W, I, ctx.fprs[RD].PS0, ctx.fprs[RD].PS1, addr);
#endif
}

static void PSQST(const u32 instr) {
    assert((HID2.pse != 0) && (HID2.lsqe != 0));

    u32 addr = (i32)(D << 20) >> 20;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    // TODO: Implement scaling
    assert(GQR[I].stscale == 0);

    switch (GQR[I].sttype) {
        case QUANT_TYPE_FLOAT:
            Write32(addr, common_FromF32((f32)ctx.fprs[RS].PS0));
            break;
        default:
            printf("Unimplemented psq_st quantization type %u\n", GQR[I].sttype);
            exit(1);
    }

    if (!W) {
        switch (GQR[I].sttype) {
            case QUANT_TYPE_FLOAT:
                Write32(addr + sizeof(f32), common_FromF32((f32)ctx.fprs[RS].PS1));
                break;
            default:
                printf("Unimplemented psq_st quantization type %u\n", GQR[I].sttype);
                exit(1);
        }
    }

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] psq_st f%u, %d(r%u), %d, %u;[%08X]: %lf, %lf\n", CIA, RS, (i32)(D << 20) >> 20, RA, W, I, addr, ctx.fprs[RS].PS0, ctx.fprs[RS].PS1);
#endif
}

static void RLWIMI(const u32 instr) {
    const u32 m = GetMask(MB, ME);

    ctx.r[RA] = (common_Rotl(ctx.r[RS], SH) & m) | (ctx.r[RA] & ~m);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] rlwimi%s r%u, r%u, %u, %u, %u; r%u: %08X, cr: %08X\n", CIA, (RC) ? "." : "", RA, RS, SH, MB, ME, RA, ctx.r[RA], CR);
#endif
}

static void RLWINM(const u32 instr) {
    ctx.r[RA] = common_Rotl(ctx.r[RS], SH) & GetMask(MB, ME);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] rlwinm%s r%u, r%u, %u, %u, %u; r%u: %08X, cr: %08X\n", CIA, (RC) ? "." : "", RA, RS, SH, MB, ME, RA, ctx.r[RA], CR);
#endif
}

static void RFI(const u32 instr) {
    (void)instr;

    printf("Broadway Return from interrupt\n");

    MSR.raw &= ~MASK_MSR;
    MSR.raw |= SRR1.raw & MASK_MSR;
    
    MSR.pow = 0;

    IA = SRR0;

    CheckInterrupt();

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] rfi; nia: %08X, msr: %08X\n", CIA, IA, MSR.raw);
#endif
}

static void SC(const u32 instr) {
    assert(GetBits(instr, 30, 30) != 0);

    SystemCall();

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sc; srr0: %08X, srr1: %08X\n", CIA, SRR0, SRR1.raw);
#endif
}

static void SLW(const u32 instr) {
    const u32 n = ctx.r[RB] & 0x3F;

    if (n >= 32) {
        ctx.r[RA] = 0;
    } else {
        ctx.r[RA] = common_Rotl(ctx.r[RS], n) & GetMask(0, 31 - n);
    }

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] slw%s r%u, r%u, r%u; r%u: %08X, cr: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA], CR);
#endif
}

static void SRW(const u32 instr) {
    const u32 n = ctx.r[RB] & 0x3F;

    if (n >= 32) {
        ctx.r[RA] = 0;
    } else {
        ctx.r[RA] = common_Rotl(ctx.r[RS], 32 - n) & GetMask(n, 31);
    }

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] srw%s r%u, r%u, r%u; r%u: %08X, cr: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA], CR);
#endif
}

static void SRAW(const u32 instr) {
    const u32 s = 0xFFFFFFFF * (ctx.r[RS] >> 31);
    const u32 n = ctx.r[RB] & 0x3F;

    const u32 r = common_Rotl(ctx.r[RS], 32 - n);
    u32 m = GetMask(n, 31);

    if (n >= 32) {
        m = 0;
    }

    XER.ca = (s != 0) && ((r & ~m) != 0);

    ctx.r[RA] = (r & m) | (s & ~m);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sraw%s r%u, r%u, r%u; r%u: %08X, cr: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA], CR);
#endif
}

static void SRAWI(const u32 instr) {
    const u32 s = 0xFFFFFFFF * (ctx.r[RS] >> 31);

    const u32 r = common_Rotl(ctx.r[RS], 32 - SH);
    const u32 m = GetMask(SH, 31);

    XER.ca = (s != 0) && ((r & ~m) != 0);

    ctx.r[RA] = (r & m) | (s & ~m);

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] srawi%s r%u, r%u, %u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RA, RS, SH, RA, ctx.r[RA], XER.raw);
#endif
}

static void STB(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write8(addr, (u8)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stb r%u, %d(r%u); [%08X]: %02X\n", CIA, RS, SIMM, RA, addr, (u8)ctx.r[RS]);
#endif
}

static void STBU(const u32 instr) {
    assert(RA != 0);

    const u32 addr = SIMM + ctx.r[RA];
    const u8 data = (u8)ctx.r[RS];

    Write8(addr, data);

    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stbu r%u, %d(r%u); [%08X]: %02X\n", CIA, RS, SIMM, RA, addr, data);
#endif
}

static void STBX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write8(addr, (u8)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stbx r%u, r%u, r%u; [%08X]: %02X\n", CIA, RS, RA, RB, addr, (u8)ctx.r[RS]);
#endif
}

static void STFD(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write64(addr, common_FromF64(ctx.fprs[RS].PS0));

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] stfd f%u, %d(r%u); [%08X]: %lf\n", CIA, RS, SIMM, RA, addr, ctx.fprs[RD].PS0);
#endif
}

static void STFIWX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, (u32)ctx.fprs[RS].raw[0]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwx fr%u, r%u, r%u; [%08X]: %08X\n", CIA, RS, RA, RB, addr, (u32)ctx.fprs[RS].raw[0]);
#endif
}

static void STFS(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, common_FromF32((f32)ctx.fprs[RS].PS0));

#ifdef BROADWAY_DEBUG_FLOATS
    printf("PPC [%08X] stfs f%u, %d(r%u); [%08X]: %lf\n", CIA, RS, SIMM, RA, addr, ctx.fprs[RD].PS0);
#endif
}

static void STH(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write16(addr, (u16)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sth r%u, %d(r%u); [%08X]: %04X\n", CIA, RS, SIMM, RA, addr, (u16)ctx.r[RS]);
#endif
}

static void STHX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write16(addr, (u16)ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sthx r%u, r%u, r%u; [%08X]: %04X\n", CIA, RS, RA, RB, addr, (u16)ctx.r[RS]);
#endif
}

static void STMW(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    for (int r = RS; r < NUM_GPRS; r++) {
        Write32(addr, ctx.r[r]);

        addr += sizeof(u32);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stmw r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, SIMM, RA, addr, ctx.r[RS]);
#endif
}

static void STSWI(const u32 instr) {
    u32 addr = 0;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    int n = RB;

    // NB = 0 encodes 32
    if (n == 0) {
        n = 32;
    }

    u32 r = RS;
    u32 i = 0;

    for (; n > 0; n--) {
        Write8(addr++, GetBits(ctx.r[r], i, i + 7));

        i = (i + 8) & 31;

        if (i == 0) {
            r = (r + 1) & (NUM_GPRS - 1);
        }
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stswi r%u, r%u, %u\n", CIA, RS, RA, RB);
#endif
}

static void STW(const u32 instr) {
    u32 addr = SIMM;

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stw r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, SIMM, RA, addr, ctx.r[RS]);
#endif
}

static void STWU(const u32 instr) {
    assert(RA != 0);

    const u32 addr = SIMM + ctx.r[RA];
    const u32 data = ctx.r[RS];

    Write32(addr, data);

    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwu r%u, %d(r%u); [%08X]: %08X\n", CIA, RS, SIMM, RA, addr, data);
#endif
}

static void STWUX(const u32 instr) {
    assert(RA != 0);

    const u32 addr = ctx.r[RA] + ctx.r[RB];

    Write32(addr, ctx.r[RS]);

    ctx.r[RA] = addr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwux r%u, r%u, r%u; [%08X]: %08X\n", CIA, RS, RA, RB, addr, ctx.r[RS]);
#endif
}

static void STWX(const u32 instr) {
    u32 addr = ctx.r[RB];

    if (RA != 0) {
        addr += ctx.r[RA];
    }

    Write32(addr, ctx.r[RS]);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] stwx r%u, r%u, r%u; [%08X]: %08X\n", CIA, RS, RA, RB, addr, ctx.r[RS]);
#endif
}

static void SUBFE(const u32 instr) {
    const u64 n = (u64)(u32)(~ctx.r[RA]) + (u64)ctx.r[RB] + (u64)XER.ca;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] subfe%s r%u, r%u, r%u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD], XER.raw);
#endif
}

static void SUBF(const u32 instr) {
    ctx.r[RD] = ctx.r[RB] - ctx.r[RA];

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] subf%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD]);
#endif
}

static void SUBFC(const u32 instr) {
    const u64 n = (u64)ctx.r[RB] - (u64)ctx.r[RA];

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] subfc%s r%u, r%u, r%u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD], XER.raw);
#endif
}

static void SUBFIC(const u32 instr) {
    const u64 n = (u64)SIMM - (u64)ctx.r[RA];

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] subfic r%u, r%u, %X; r%u: %08X, xer: %08X\n", CIA, RD, RA, UIMM, RD, ctx.r[RD], XER.raw);
#endif
}

static void SUBFZE(const u32 instr) {
    const u64 n = (u64)(u32)(~ctx.r[RA]) + (u64)XER.ca;

    XER.ca = (n >> 32) & 1;

    ctx.r[RD] = (u32)n;

    if (RC) {
        SetFlags(0, ctx.r[RD]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] subfze%s r%u, r%u, r%u; r%u: %08X, xer: %08X\n", CIA, (RC) ? "." : "", RD, RA, RB, RD, ctx.r[RD], XER.raw);
#endif
}

static void SYNC(const u32 instr) {
    (void)instr;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] sync\n", CIA);
#endif
}

static void XOR(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] ^ ctx.r[RB];

    if (RC) {
        SetFlags(0, ctx.r[RA]);
    }

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] xor%s r%u, r%u, r%u; r%u: %08X\n", CIA, (RC) ? "." : "", RA, RS, RB, RA, ctx.r[RA]);
#endif
}

static void XORI(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] ^ UIMM;

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] xori r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void XORIS(const u32 instr) {
    ctx.r[RA] = ctx.r[RS] ^ (UIMM << 16);

#ifdef BROADWAY_DEBUG
    printf("PPC [%08X] xoris r%u, r%u, %X; r%u: %08X\n", CIA, RA, RS, UIMM, RA, ctx.r[RA]);
#endif
}

static void ExecInstr(const u32 instr) {
    switch (OPCD) {
        case PRIMARY_PAIREDSINGLE:
            switch (XO) {
                case PAIREDSINGLE_PSMR:
                    PSMR(instr);
                    break;
                case PAIREDSINGLE_PSMERGE01:
                    PSMERGE01(instr);
                    break;
                case PAIREDSINGLE_PSMERGE10:
                    PSMERGE10(instr);
                    break;
                default:
                    printf("Unimplemented Broadway Paired Single opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_MULLI:
            MULLI(instr);
            break;
        case PRIMARY_SUBFIC:
            SUBFIC(instr);
            break;
        case PRIMARY_CMPLI:
            CMPLI(instr);
            break;
        case PRIMARY_CMPI:
            CMPI(instr);
            break;
        case PRIMARY_ADDIC:
            ADDIC(instr);
            break;
        case PRIMARY_ADDICrc:
            ADDICrc(instr);
            break;
        case PRIMARY_ADDI:
            ADDI(instr);
            break;
        case PRIMARY_ADDIS:
            ADDIS(instr);
            break;
        case PRIMARY_BC:
            BC(instr);
            break;
        case PRIMARY_SC:
            SC(instr);
            break;
        case PRIMARY_B:
            B(instr);
            break;
        case PRIMARY_SYSTEM:
            switch (XO) {
                case SYSTEM_MCRF:
                    MCRF(instr);
                    break;
                case SYSTEM_BCLR:
                    BCLR(instr);
                    break;
                case SYSTEM_CRNOR:
                    CRNOR(instr);
                    break;
                case SYSTEM_RFI:
                    RFI(instr);
                    break;
                case SYSTEM_ISYNC:
                    ISYNC(instr);
                    break;
                case SYSTEM_CRXOR:
                    CRXOR(instr);
                    break;
                case SYSTEM_CREQV:
                    CREQV(instr);
                    break;
                case SYSTEM_BCCTR:
                    BCCTR(instr);
                    break;
                default:
                    printf("Unimplemented Broadway system opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_RLWIMI:
            RLWIMI(instr);
            break;
        case PRIMARY_RLWINM:
            RLWINM(instr);
            break;
        case PRIMARY_ORI:
            ORI(instr);
            break;
        case PRIMARY_ORIS:
            ORIS(instr);
            break;
        case PRIMARY_XORI:
            XORI(instr);
            break;
        case PRIMARY_XORIS:
            XORIS(instr);
            break;
        case PRIMARY_ANDIrc:
            ANDIrc(instr);
            break;
        case PRIMARY_ANDISrc:
            ANDISrc(instr);
            break;
        case PRIMARY_REGISTER:
            switch (XO) {
                case SECONDARY_CMP:
                    CMP(instr);
                    break;
                case SECONDARY_SUBFC:
                    SUBFC(instr);
                    break;
                case SECONDARY_ADDC:
                    ADDC(instr);
                    break;
                case SECONDARY_MULHWU:
                    MULHWU(instr);
                    break;
                case SECONDARY_MFCR:
                    MFCR(instr);
                    break;
                case SECONDARY_LWZX:
                    LWZX(instr);
                    break;
                case SECONDARY_SLW:
                    SLW(instr);
                    break;
                case SECONDARY_CNTLZW:
                    CNTLZW(instr);
                    break;
                case SECONDARY_AND:
                    AND(instr);
                    break;
                case SECONDARY_CMPL:
                    CMPL(instr);
                    break;
                case SECONDARY_SUBF:
                    SUBF(instr);
                    break;
                case SECONDARY_LWZUX:
                    LWZUX(instr);
                    break;
                case SECONDARY_ANDC:
                    ANDC(instr);
                    break;
                case SECONDARY_MULHW:
                    MULHW(instr);
                    break;
                case SECONDARY_MFMSR:
                    MFMSR(instr);
                    break;
                case SECONDARY_DCBF:
                    DCBF(instr);
                    break;
                case SECONDARY_LBZX:
                    LBZX(instr);
                    break;
                case SECONDARY_NEG:
                    NEG(instr);
                    break;
                case SECONDARY_NOR:
                    NOR(instr);
                    break;
                case SECONDARY_SUBFE:
                    SUBFE(instr);
                    break;
                case SECONDARY_ADDE:
                    ADDE(instr);
                    break;
                case SECONDARY_MTCR:
                    MTCR(instr);
                    break;
                case SECONDARY_MTMSR:
                    MTMSR(instr);
                    break;
                case SECONDARY_STWX:
                    STWX(instr);
                    break;
                case SECONDARY_STWUX:
                    STWUX(instr);
                    break;
                case SECONDARY_SUBFZE:
                    SUBFZE(instr);
                    break;
                case SECONDARY_ADDZE:
                    ADDZE(instr);
                    break;
                case SECONDARY_MTSR:
                    MTSR(instr);
                    break;
                case SECONDARY_STBX:
                    STBX(instr);
                    break;
                case SECONDARY_MULLW:
                    MULLW(instr);
                    break;
                case SECONDARY_ADD:
                    ADD(instr);
                    break;
                case SECONDARY_LHZX:
                    LHZX(instr);
                    break;
                case SECONDARY_XOR:
                    XOR(instr);
                    break;
                case SECONDARY_MFSPR:
                    MFSPR(instr);
                    break;
                case SECONDARY_MFTB:
                    MFTB(instr);
                    break;
                case SECONDARY_STHX:
                    STHX(instr);
                    break;
                case SECONDARY_ORC:
                    ORC(instr);
                    break;
                case SECONDARY_OR:
                    OR(instr);
                    break;
                case SECONDARY_DIVWU:
                    DIVWU(instr);
                    break;
                case SECONDARY_MTSPR:
                    MTSPR(instr);
                    break;
                case SECONDARY_DCBI:
                    DCBI(instr);
                    break;
                case SECONDARY_DIVW:
                    DIVW(instr);
                    break;
                case SECONDARY_SRW:
                    SRW(instr);
                    break;
                case SECONDARY_LSWI:
                    LSWI(instr);
                    break;
                case SECONDARY_SYNC:
                    SYNC(instr);
                    break;
                case SECONDARY_LFDX:
                    LFDX(instr);
                    break;
                case SECONDARY_STSWI:
                    STSWI(instr);
                    break;
                case SECONDARY_SRAW:
                    SRAW(instr);
                    break;
                case SECONDARY_SRAWI:
                    SRAWI(instr);
                    break;
                case SECONDARY_EXTSH:
                    EXTSH(instr);
                    break;
                case SECONDARY_EXTSB:
                    EXTSB(instr);
                    break;
                case SECONDARY_ICBI:
                    ICBI(instr);
                    break;
                case SECONDARY_STFIWX:
                    STFIWX(instr);
                    break;
                case SECONDARY_DCBZ:
                    DCBZ(instr);
                    break;
                default:
                    printf("Unimplemented Broadway secondary opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                    exit(1);
            }
            break;
        case PRIMARY_LWZ:
            LWZ(instr);
            break;
        case PRIMARY_LWZU:
            LWZU(instr);
            break;
        case PRIMARY_LBZ:
            LBZ(instr);
            break;
        case PRIMARY_LBZU:
            LBZU(instr);
            break;
        case PRIMARY_STW:
            STW(instr);
            break;
        case PRIMARY_STWU:
            STWU(instr);
            break;
        case PRIMARY_STB:
            STB(instr);
            break;
        case PRIMARY_STBU:
            STBU(instr);
            break;
        case PRIMARY_LHZ:
            LHZ(instr);
            break;
        case PRIMARY_LHA:
            LHA(instr);
            break;
        case PRIMARY_STH:
            STH(instr);
            break;
        case PRIMARY_LMW:
            LMW(instr);
            break;
        case PRIMARY_STMW:
            STMW(instr);
            break;
        case PRIMARY_LFS:
            LFS(instr);
            break;
        case PRIMARY_LFD:
            LFD(instr);
            break;
        case PRIMARY_STFS:
            STFS(instr);
            break;
        case PRIMARY_STFD:
            STFD(instr);
            break;
        case PRIMARY_PSQL:
            PSQL(instr);
            break;
        case PRIMARY_PSQST:
            PSQST(instr);
            break;
        case PRIMARY_FLOAT:
            switch (FXO) {
                case FLOAT_FDIV:
                    FDIV(instr);
                    break;
                case FLOAT_FSUB:
                    FSUB(instr);
                    break;
                case FLOAT_FADD:
                    FADD(instr);
                    break;
                case FLOAT_FMUL:
                    FMUL(instr);
                    break;
                case FLOAT_FMSUB:
                    FMSUB(instr);
                    break;
                case FLOAT_FMADD:
                    FMADD(instr);
                    break;
                default:
                    switch (XO) {
                        case FLOAT_FCMPU:
                            FCMPU(instr);
                            break;
                        case FLOAT_FCTIWZ:
                            FCTIWZ(instr);
                            break;
                        case FLOAT_MTFSB1:
                            MTFSB1(instr);
                            break;
                        case FLOAT_FNEG:
                            FNEG(instr);
                            break;
                        case FLOAT_FMR:
                            FMR(instr);
                            break;
                        case FLOAT_MTFSF:
                            MTFSF(instr);
                            break;
                        default:
                            printf("Unimplemented Broadway float opcode %u (IA: %08X, instruction: %08X)\n", XO, CIA, instr);

                            exit(1);
                    }
            }
            break;
        default:
            printf("Unimplemented Broadway primary opcode %u (IA: %08X, instruction: %08X)\n", OPCD, CIA, instr);

            exit(1);
    }
}

static void IncrementTbr() {
    static int prescaler = 0;

    prescaler++;

    if (prescaler >= 12) {
        prescaler = 0;

        TBR++;
    }
}

void broadway_Initialize() {

}

void broadway_Reset() {
    memset(&ctx, 0, sizeof(ctx));
}

void broadway_Shutdown() {

}

void broadway_Run() {
    for (; ctx.cyclesToRun > 0; ctx.cyclesToRun--) {
        const u32 instr = FetchInstr();

        ExecInstr(instr);

        IncrementTbr();
    }
}

void broadway_SetEntry(const u32 addr) {
    IA = addr;
}

void broadway_TryInterrupt() {
    if (MSR.ee != 0) {
        ExternalInterrupt();
    }
}

i64* broadway_GetCyclesToRun() {
    return &ctx.cyclesToRun;
}
