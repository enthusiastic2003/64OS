#ifndef VMM_EXTRA_H
#define VMM_EXTRA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine_requests.h"  // For HHDM offset, etc.
#include "pmm_mngr.h"         // Assumes phys_addr_t is defined here

#ifdef __cplusplus
extern "C" {
#endif

/* Virtual address type definition */
typedef uint64_t virt_addr_t;

/* Structure to hold mapping information */
typedef struct {
    phys_addr_t phys_addr;  /* The physical address mapped */
    uint64_t flags;         /* The page flags (e.g., PAGE_PRESENT | PAGE_WRITE) */
} mapping_info_t;

/**
 * vmm_map_range - Map a contiguous range of virtual addresses to physical addresses.
 *
 * @start:      The starting virtual address.
 * @size:       The size of the region in bytes.
 * @phys_start: The starting physical address.
 * @flags:      Flags for each mapping (e.g., PAGE_PRESENT | PAGE_WRITE | PAGE_USER).
 */
void vmm_map_range(virt_addr_t start, size_t size, phys_addr_t phys_start, uint64_t flags);

/**
 * vmm_unmap_range - Unmap a contiguous range of virtual addresses.
 *
 * @start: The starting virtual address.
 * @size:  The size of the region in bytes.
 */
void vmm_unmap_range(virt_addr_t start, size_t size);

/**
 * vmm_query_mapping - Retrieve mapping details for a virtual address.
 *
 * @virt_addr: The virtual address to query.
 *
 * Returns a mapping_info_t structure containing the physical address and flags.
 */
mapping_info_t vmm_query_mapping(virt_addr_t virt_addr);

/**
 * vmm_change_flags - Modify the flags for an existing mapping.
 *
 * @virt_addr: The virtual address whose mapping is to be modified.
 * @new_flags: The new flag bits (in addition to PAGE_PRESENT) to apply.
 */
void vmm_change_flags(virt_addr_t virt_addr, uint64_t new_flags);

/**
 * vmm_dump_page_tables - Dump the current state of the page tables.
 *
 * For debugging purposes, this function iterates through the page table hierarchy
 * (using the recursive mapping) and prints non-empty entries.
 */
void vmm_dump_page_tables(void);

#ifdef __cplusplus
}
#endif

#endif /* VMM_EXTRA_H */
