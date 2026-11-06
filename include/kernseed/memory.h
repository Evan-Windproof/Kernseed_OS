/* SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 */
#ifndef KERNSEED_MEMORY_H
#define KERNSEED_MEMORY_H
#include <stdint.h>
#include <stdbool.h>
#define PAGE_SIZE 4096ULL
#define VM_BASE 0xffffc00000000000ULL
#define VM_SIZE (1ULL << 30)
#define VM_WRITE 2ULL
#define VM_NX (1ULL << 63)
struct memory_stats { uint64_t hhdm, cr3, usable, free, allocated, tables, mapped, tests; };
extern volatile struct memory_stats memory_state;
enum vmm_error_code { MEM_OK, MEM_INVALID, MEM_OWNERSHIP, MEM_CONFLICT, MEM_HUGE, MEM_OOM };
extern volatile enum vmm_error_code vmm_error;
bool memory_span(uint64_t base,uint64_t length,bool reserved,uint64_t *first,uint64_t *last);
bool memory_init(void);
uint64_t pmm_alloc(uint64_t pages);
bool pmm_free(uint64_t physical);
bool pmm_allocated(uint64_t physical);
bool vmm_query(uint64_t virtual_address, uint64_t *physical, uint64_t *flags);
bool vmm_map(uint64_t virtual_address, uint64_t physical, uint64_t flags);
bool vmm_unmap(uint64_t virtual_address);
void *physical_pointer(uint64_t physical);
bool memory_selftest(void);
void memory_fault_test(void);
#endif
