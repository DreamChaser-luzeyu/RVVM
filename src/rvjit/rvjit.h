/*
rvjit.h - Retargetable Versatile JIT Compiler
Copyright (C) 2021  LekKit <github.com/LekKit>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef RVJIT_H
#define RVJIT_H

#include "rvvm_types.h"
#include "compiler.h"
#include "utils.h"
#include "hashmap.h"
#include "vector.h"
#include <string.h>

#define REG_ILL 0xFF // Register is not allocated

// RISC-V register allocator details
#define RVJIT_REGISTERS 32
#define RVJIT_REGISTER_ZERO 0

#if defined(__x86_64__) || defined(_M_AMD64)
    #ifdef _WIN32
        #ifdef __MINGW64__
            #define RVJIT_CALL __attribute__((sysv_abi))
            #define RVJIT_ABI_SYSV 1
        #else
            #define RVJIT_ABI_WIN64 1
        #endif
    #else
        #if GNU_ATTRIBUTE(sysv_abi)
            #define RVJIT_CALL __attribute__((sysv_abi))
        #endif
        #define RVJIT_ABI_SYSV 1
    #endif
    #define RVJIT_NATIVE_64BIT 1
    #define RVJIT_NATIVE_LINKER 1
    #define RVJIT_X86 1
#elif defined(__i386__) || defined(_M_IX86)
    #ifdef _WIN32
        #ifdef __MINGW32__
            #define RVJIT_CALL __attribute__((fastcall))
        #else
            #define RVJIT_CALL __fastcall
        #endif
    #else
        #define RVJIT_CALL __attribute__((fastcall))
    #endif
    #define RVJIT_ABI_FASTCALL 1
    #define RVJIT_NATIVE_LINKER 1
    #define RVJIT_X86 1
#elif defined(__riscv)
    #if __riscv_xlen == 64
        #define RVJIT_NATIVE_64BIT 1
    #elif __riscv_xlen != 32
        #error No JIT support for RV128!
    #endif
    #define RVJIT_ABI_SYSV 1
    #define RVJIT_NATIVE_LINKER 1
    #define RVJIT_RISCV 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define RVJIT_NATIVE_64BIT 1
    #define RVJIT_ABI_SYSV 1
    #define RVJIT_NATIVE_LINKER 1
    #define RVJIT_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define RVJIT_ABI_SYSV 1
    #define RVJIT_ARM 1
#else
    #error No JIT support for the target platform!!!
#endif

// No specific calling convention requirements
#ifndef RVJIT_CALL
#define RVJIT_CALL
#endif

typedef void (* RVJIT_CALL rvjit_func_t)(void* vm);
typedef uint8_t regflags_t;
typedef size_t  branch_t;

#define BRANCH_NEW ((branch_t)-1)
#define BRANCH_ENTRY  false
#define BRANCH_TARGET true

#define LINKAGE_NONE 0
#define LINKAGE_TAIL 1
#define LINKAGE_JMP  2

typedef struct {
    uint8_t* data;          ///< Data Cache
    const uint8_t* code;    ///< JIT Code Cache
    size_t curr;
    size_t size;
    hashmap_t blocks;       ///< size 64 `block->phys_pc` to `code`
    hashmap_t block_links;  ///< size 64

    // Dirty memory tracking
    uint32_t* dirty_pages;
    size_t    dirty_mask;
} rvjit_heap_t;

typedef struct {
    size_t last_used;   // Last usage of register for LRU reclaim
    int32_t auipc_off;
    regid_t hreg;       // Claimed host register, REG_ILL if not mapped
    regflags_t flags;   // Register allocation details
} rvjit_reginfo_t;


/**
 * Act as the value of TLB cache
 */
typedef struct {
    rvjit_heap_t heap;
    vector_t(struct {paddr_t dest; size_t ptr;}) links;
    uint8_t* code;
    size_t size;
    size_t space;
    size_t hreg_mask;        // Bitmask of available non-clobbered host registers
    size_t abireclaim_mask;  // Bitmask of reclaimed abi-clobbered host registers to restore
    rvjit_reginfo_t regs[RVJIT_REGISTERS];
    vaddr_t virt_pc;
    paddr_t phys_pc;        ///< PC that JIT operates on
    int32_t pc_off;
    bool rv64;
    uint8_t linkage;
} rvjit_block_t;

// Creates JIT context, sets upper limit on cache size
bool rvjit_ctx_init(rvjit_block_t* block, size_t heap_size);

// Frees the JIT context and block cache
// All functions generated by this context are invalid after freeing it!
void rvjit_ctx_free(rvjit_block_t* block);

// Set guest bitness
static inline void rvjit_set_rv64(rvjit_block_t* block, bool rv64)
{
#ifdef RVJIT_NATIVE_64BIT
    block->rv64 = rv64;
#else
    UNUSED(rv64);
    block->rv64 = false;
#endif
}

// Creates a new block, prepares codegen
void rvjit_block_init(rvjit_block_t* block);

// Returns true if the block has some instructions emitted
static inline bool rvjit_block_nonempty(rvjit_block_t* block)
{
    return block->size != 0;
}

// Returns NULL when cache is full, otherwise returns a valid function pointer
// Inserts block into the lookup cache by phys_pc key
rvjit_func_t rvjit_block_finalize(rvjit_block_t* block);

// Looks up for compiled block by phys_pc, returns NULL when no block was found
rvjit_func_t rvjit_block_lookup(rvjit_block_t* block, paddr_t phys_pc);

// Track dirty memory to transparently invalidate JIT caches
void rvjit_init_memtracking(rvjit_block_t* block, size_t size);
void rvjit_mark_dirty_mem(rvjit_block_t* block, paddr_t addr, size_t size);

// Cleans up internal heap & lookup cache entirely
void rvjit_flush_cache(rvjit_block_t* block);

// Internal APIs

void rvjit_emit_init(rvjit_block_t* block);
void rvjit_emit_end(rvjit_block_t* block, uint8_t linkage);

regid_t rvjit_reclaim_hreg(rvjit_block_t* block);

static inline size_t rvjit_hreg_mask(regid_t hreg)
{
    return (1ULL << hreg);
}

static inline void rvjit_put_code(rvjit_block_t* block, const void* inst, size_t size)
{
    if (unlikely(block->space < block->size + size)) {
        block->space += 1024;
        block->code = safe_realloc(block->code, block->space);
    }
    memcpy(block->code + block->size, inst, size);
    block->size += size;
}

// Frees explicitly claimed hardware register
static inline void rvjit_free_hreg(rvjit_block_t* block, regid_t hreg)
{
    block->hreg_mask |= rvjit_hreg_mask(hreg);
}

static inline regid_t rvjit_try_claim_hreg(rvjit_block_t* block)
{
    for (regid_t i=0; i<RVJIT_REGISTERS; ++i) {
        if (block->hreg_mask & rvjit_hreg_mask(i)) {
            block->hreg_mask &= ~rvjit_hreg_mask(i);
            return i;
        }
    }
    return REG_ILL;
}

// Claims any free hardware register, or reclaims mapped register preserving it's value in VM
static inline regid_t rvjit_claim_hreg(rvjit_block_t* block)
{
    regid_t hreg = rvjit_try_claim_hreg(block);
    // No free host registers
    if (hreg == REG_ILL) {
        hreg = rvjit_reclaim_hreg(block);
    }
    return hreg;
}

#endif
