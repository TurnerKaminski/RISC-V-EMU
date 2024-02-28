// Copyright © 2019-2023
// 
// Licensed under the Apache License;
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#define XLEN 32

#define MAX_NUM_REGS 32

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 3
#endif

#ifndef RAM_PAGE_SIZE
#define RAM_PAGE_SIZE 4096
#endif

#ifndef MEM_CYCLE_RATIO
#define MEM_CYCLE_RATIO -1
#endif

#ifndef MEMORY_BANKS
#define MEMORY_BANKS 2
#endif

#ifdef L1_DISABLE
    #define ICACHE_DISABLE
    #define DCACHE_DISABLE
#endif

#ifndef MEM_BLOCK_SIZE
#define MEM_BLOCK_SIZE 64
#endif

#ifndef MEM_ADDR_WIDTH
#ifdef XLEN_64
#define MEM_ADDR_WIDTH 48
#else
#define MEM_ADDR_WIDTH 32
#endif
#endif

#define L1_LINE_SIZE MEM_BLOCK_SIZE

#ifndef STARTUP_ADDR
#define STARTUP_ADDR 0x80000000
#endif

#ifndef STACK_BASE_ADDR
#define STACK_BASE_ADDR 0xFF000000
#endif

#ifndef IO_BASE_ADDR
#define IO_BASE_ADDR STACK_BASE_ADDR
#endif

#ifndef IO_COUT_ADDR
#define IO_COUT_ADDR IO_BASE_ADDR
#endif
#define IO_COUT_SIZE MEM_BLOCK_SIZE

#ifndef IO_CSR_ADDR
#define IO_CSR_ADDR (IO_COUT_ADDR + IO_COUT_SIZE)
#endif
#define IO_CSR_SIZE (4 * 64 * NUM_CORES * NUM_CLUSTERS)

#ifndef STACK_LOG2_SIZE
#define STACK_LOG2_SIZE 13
#endif
#define STACK_SIZE (1 << STACK_LOG2_SIZE)

// Pipeline Configuration /////////////////////////////////////////////////////

// Size of Instruction Buffer
#ifndef IBUF_SIZE
#define IBUF_SIZE   (2 * (NUM_WARPS / ISSUE_WIDTH))
#endif

// Size of LSU Request Queue
#ifndef LSUQ_SIZE
#define LSUQ_SIZE   (2 * (NUM_THREADS / NUM_LSU_LANES))
#endif

// LSU Duplicate Address Check
#ifndef LSU_DUP_DISABLE
#define LSU_DUP_ENABLE
#endif
#ifdef LSU_DUP_ENABLE
#define LSU_DUP_ENABLED 1
#else
#define LSU_DUP_ENABLED 0
#endif

// Icache Configurable Knobs //////////////////////////////////////////////////

// Cache Enable
#ifndef ICACHE_DISABLE
#define ICACHE_ENABLE
#endif
#ifdef ICACHE_ENABLE
    #define ICACHE_ENABLED 1
#else
    #define ICACHE_ENABLED 0
    #define NUM_ICACHES 0
#endif

// Number of Cache Units
#ifndef NUM_ICACHES
#define NUM_ICACHES UP(SOCKET_SIZE / 4)
#endif

// Cache Size
#ifndef ICACHE_SIZE
#define ICACHE_SIZE 16384
#endif

// Number of Banks
#ifndef ICACHE_NUM_BANKS
#define ICACHE_NUM_BANKS 1
#endif

// Core Response Queue Size
#ifndef ICACHE_CRSQ_SIZE
#define ICACHE_CRSQ_SIZE 2
#endif

// Miss Handling Register Size
#ifndef ICACHE_MSHR_SIZE
#define ICACHE_MSHR_SIZE 16
#endif

// Memory Request Queue Size
#ifndef ICACHE_MREQ_SIZE
#define ICACHE_MREQ_SIZE 4
#endif

// Memory Response Queue Size
#ifndef ICACHE_MRSQ_SIZE
#define ICACHE_MRSQ_SIZE 0
#endif

// Number of Associative Ways
#ifndef ICACHE_NUM_WAYS
#define ICACHE_NUM_WAYS 1
#endif

// Dcache Configurable Knobs //////////////////////////////////////////////////

// Cache Enable
#ifndef DCACHE_DISABLE
#define DCACHE_ENABLE
#endif
#ifdef DCACHE_ENABLE
    #define DCACHE_ENABLED 1
#else
    #define DCACHE_ENABLED 0
    #define NUM_DCACHES 0
    #define DCACHE_NUM_BANKS 1
#endif

// Number of Cache Units
#ifndef NUM_DCACHES
#define NUM_DCACHES UP(SOCKET_SIZE / 4)
#endif

// Cache Size
#ifndef DCACHE_SIZE
#define DCACHE_SIZE 16384
#endif

// Number of Banks
#ifndef DCACHE_NUM_BANKS
#define DCACHE_NUM_BANKS 1
#endif

// Core Response Queue Size
#ifndef DCACHE_CRSQ_SIZE
#define DCACHE_CRSQ_SIZE 2
#endif

// Miss Handling Register Size
#ifndef DCACHE_MSHR_SIZE
#define DCACHE_MSHR_SIZE 16
#endif

// Memory Request Queue Size
#ifndef DCACHE_MREQ_SIZE
#define DCACHE_MREQ_SIZE 4
#endif

// Memory Response Queue Size
#ifndef DCACHE_MRSQ_SIZE
#define DCACHE_MRSQ_SIZE 0
#endif

// Number of Associative Ways
#ifndef DCACHE_NUM_WAYS
#define DCACHE_NUM_WAYS 1
#endif

// ISA Extensions /////////////////////////////////////////////////////////////

#ifdef EXT_A_ENABLE
    #define EXT_A_ENABLED   1
#else
    #define EXT_A_ENABLED   0
#endif

#ifdef EXT_C_ENABLE
    #define EXT_C_ENABLED   1
#else
    #define EXT_C_ENABLED   0
#endif

#ifdef EXT_D_ENABLE
    #define EXT_D_ENABLED   1
#else
    #define EXT_D_ENABLED   0
#endif

#ifdef EXT_F_ENABLE
    #define EXT_F_ENABLED   1
#else
    #define EXT_F_ENABLED   0
#endif

#ifdef EXT_M_ENABLE
    #define EXT_M_ENABLED   1
#else
    #define EXT_M_ENABLED   0
#endif

#define ISA_STD_A           0
#define ISA_STD_C           2
#define ISA_STD_D           3
#define ISA_STD_E           4
#define ISA_STD_F           5
#define ISA_STD_H           7
#define ISA_STD_I           8
#define ISA_STD_N           13
#define ISA_STD_Q           16
#define ISA_STD_S           18
#define ISA_STD_U           20

#define ISA_EXT_ICACHE      0
#define ISA_EXT_DCACHE      1
#define ISA_EXT_L2CACHE     2
#define ISA_EXT_L3CACHE     3
#define ISA_EXT_SMEM        4

#define MISA_EXT  (ICACHE_ENABLED  << ISA_EXT_ICACHE) \
                | (DCACHE_ENABLED  << ISA_EXT_DCACHE) \
                | (L2_ENABLED      << ISA_EXT_L2CACHE) \
                | (L3_ENABLED      << ISA_EXT_L3CACHE) \
                | (SM_ENABLED      << ISA_EXT_SMEM)

#define MISA_STD  (EXT_A_ENABLED <<  0) /* A - Atomic Instructions extension */ \
                | (0 <<  1) /* B - Tentatively reserved for Bit operations extension */ \
                | (EXT_C_ENABLED <<  2) /* C - Compressed extension */ \
                | (EXT_D_ENABLED <<  3) /* D - Double precsision floating-point extension */ \
                | (0 <<  4) /* E - RV32E base ISA */ \
                | (EXT_F_ENABLED << 5) /* F - Single precsision floating-point extension */ \
                | (0 <<  6) /* G - Additional standard extensions present */ \
                | (0 <<  7) /* H - Hypervisor mode implemented */ \
                | (1 <<  8) /* I - RV32I/64I/128I base ISA */ \
                | (0 <<  9) /* J - Reserved */ \
                | (0 << 10) /* K - Reserved */ \
                | (0 << 11) /* L - Tentatively reserved for Bit operations extension */ \
                | (EXT_M_ENABLED << 12) /* M - Integer Multiply/Divide extension */ \
                | (0 << 13) /* N - User level interrupts supported */ \
                | (0 << 14) /* O - Reserved */ \
                | (0 << 15) /* P - Tentatively reserved for Packed-SIMD extension */ \
                | (0 << 16) /* Q - Quad-precision floating-point extension */ \
                | (0 << 17) /* R - Reserved */ \
                | (0 << 18) /* S - Supervisor mode implemented */ \
                | (0 << 19) /* T - Tentatively reserved for Transactional Memory extension */ \
                | (1 << 20) /* U - User mode implemented */ \
                | (0 << 21) /* V - Tentatively reserved for Vector extension */ \
                | (0 << 22) /* W - Reserved */ \
                | (1 << 23) /* X - Non-standard extensions present */ \
                | (0 << 24) /* Y - Reserved */ \
                | (0 << 25) /* Z - Reserved */

// Device identification //////////////////////////////////////////////////////

#define VENDOR_ID           0
#define ARCHITECTURE_ID     0
#define IMPLEMENTATION_ID   0

#define VX_CSR_ADDR_BITS                12
#define VX_DCR_ADDR_BITS                12

#define VX_DCR_BASE_STATE_BEGIN         0x001
#define VX_DCR_BASE_STARTUP_ADDR0       0x001
#define VX_DCR_BASE_STARTUP_ADDR1       0x002
#define VX_DCR_BASE_MPM_CLASS           0x003
#define VX_DCR_BASE_STATE_END           0x004

#define VX_DCR_BASE_STATE(addr)         ((addr) - VX_DCR_BASE_STATE_BEGIN)
#define VX_DCR_BASE_STATE_COUNT         (VX_DCR_BASE_STATE_END-VX_DCR_BASE_STATE_BEGIN)

// Machine Performance-monitoring counters classes

#define VX_DCR_MPM_CLASS_NONE           0           
#define VX_DCR_MPM_CLASS_CORE           1
#define VX_DCR_MPM_CLASS_MEM            2

// User Floating-Point CSRs

#define VX_CSR_FFLAGS                   0x001
#define VX_CSR_FRM                      0x002
#define VX_CSR_FCSR                     0x003
 
#define VX_CSR_SATP                     0x180

#define VX_CSR_PMPCFG0                  0x3A0
#define VX_CSR_PMPADDR0                 0x3B0

#define VX_CSR_MSTATUS                  0x300
#define VX_CSR_MISA                     0x301
#define VX_CSR_MEDELEG                  0x302
#define VX_CSR_MIDELEG                  0x303
#define VX_CSR_MIE                      0x304
#define VX_CSR_MTVEC                    0x305

#define VX_CSR_MEPC                     0x341

#define VX_CSR_MNSTATUS                 0x744

#define VX_CSR_MPM_BASE                 0xB00
#define VX_CSR_MPM_BASE_H               0xB80
#define VX_CSR_MPM_USER                 0xB03
#define VX_CSR_MPM_USER_H               0xB83

// Machine Performance-monitoring core counters
// PERF: Standard
#define VX_CSR_MCYCLE                   0xB00
#define VX_CSR_MCYCLE_H                 0xB80
#define VX_CSR_MPM_RESERVED             0xB01
#define VX_CSR_MPM_RESERVED_H           0xB81
#define VX_CSR_MINSTRET                 0xB02
#define VX_CSR_MINSTRET_H               0xB82

// PERF: memory
#define VX_CSR_MPM_IFETCHES             0xB0B
#define VX_CSR_MPM_IFETCHES_H           0xB8B
#define VX_CSR_MPM_LOADS                0xB0C
#define VX_CSR_MPM_LOADS_H              0xB8C
#define VX_CSR_MPM_STORES               0xB0D
#define VX_CSR_MPM_STORES_H             0xB8D
#define VX_CSR_MPM_IFETCH_LT            0xB0E
#define VX_CSR_MPM_IFETCH_LT_H          0xB8E
#define VX_CSR_MPM_LOAD_LT              0xB0F 
#define VX_CSR_MPM_LOAD_LT_H            0xB8F

// Machine Performance-monitoring memory counters
// PERF: icache
#define VX_CSR_MPM_ICACHE_READS         0xB03     // total reads
#define VX_CSR_MPM_ICACHE_READS_H       0xB83
#define VX_CSR_MPM_ICACHE_MISS_R        0xB04     // read misses
#define VX_CSR_MPM_ICACHE_MISS_R_H      0xB84
#define VX_CSR_MPM_ICACHE_MSHR_ST       0xB05     // MSHR stalls
#define VX_CSR_MPM_ICACHE_MSHR_ST_H     0xB85
// PERF: dcache
#define VX_CSR_MPM_DCACHE_READS         0xB06     // total reads
#define VX_CSR_MPM_DCACHE_READS_H       0xB86
#define VX_CSR_MPM_DCACHE_WRITES        0xB07     // total writes
#define VX_CSR_MPM_DCACHE_WRITES_H      0xB87
#define VX_CSR_MPM_DCACHE_MISS_R        0xB08     // read misses
#define VX_CSR_MPM_DCACHE_MISS_R_H      0xB88
#define VX_CSR_MPM_DCACHE_MISS_W        0xB09     // write misses
#define VX_CSR_MPM_DCACHE_MISS_W_H      0xB89
#define VX_CSR_MPM_DCACHE_BANK_ST       0xB0A     // bank conflicts
#define VX_CSR_MPM_DCACHE_BANK_ST_H     0xB8A
#define VX_CSR_MPM_DCACHE_MSHR_ST       0xB0B     // MSHR stalls
#define VX_CSR_MPM_DCACHE_MSHR_ST_H     0xB8B
// PERF: memory
#define VX_CSR_MPM_MEM_READS            0xB18     // total reads
#define VX_CSR_MPM_MEM_READS_H          0xB98
#define VX_CSR_MPM_MEM_WRITES           0xB19     // total writes
#define VX_CSR_MPM_MEM_WRITES_H         0xB99
#define VX_CSR_MPM_MEM_LT               0xB1A     // memory latency
#define VX_CSR_MPM_MEM_LT_H             0xB9A

// Machine Information Registers

#define VX_CSR_MVENDORID                0xF11
#define VX_CSR_MARCHID                  0xF12
#define VX_CSR_MIMPID                   0xF13
#define VX_CSR_MHARTID                  0xF14

// GPGU CSRs

#define VX_CSR_THREAD_ID                0xCC0
#define VX_CSR_WARP_ID                  0xCC1
#define VX_CSR_CORE_ID                  0xCC2
#define VX_CSR_WARP_MASK                0xCC3
#define VX_CSR_THREAD_MASK              0xCC4     // warning! this value is also used in LLVM

#define VX_CSR_NUM_THREADS              0xFC0
#define VX_CSR_NUM_WARPS                0xFC1
#define VX_CSR_NUM_CORES                0xFC2
