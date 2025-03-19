#ifndef VMM_H
#define VMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "limine_requests.h"

/* Page size definition */
#define PAGE_SIZE 4096

/* Common page table flags */
#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4
#define PAGE_SIZE_2MB 0x80

/* PML4 indices reserved for specific purposes */
#define RECURSIVE_INDEX 510   /* Used for the self-referencing (recursive) mapping */
#define KERNEL_INDEX    511   /* Reserved for kernel/HHDM mappings, for example */
#define HHDM_INDEX      256   /* Used for HHDM mappings */

#define STACK_INDEX     257   /* Used for stack mappings */
#define FRAMEBUFFER_INDEX 258 /* Used for framebuffer mappings */


/* Define the HHDM offset (adjust this based on your system's configuration) */
#define HHDM_OFFSET (hhdm_request.response->offset)

/* Base address for the recursive mapping region */
#define RECURSIVE_BASE ((uint64_t)RECURSIVE_INDEX << 39)

#ifdef __cplusplus
extern "C" {
#endif

/* Assume phys_addr_t is defined in pmm_mngr.h */
typedef uint64_t virt_addr_t;

/**
 * get_pte_ptr - Get pointer to the page table entry for a given virtual address.
 *
 * @virt_addr: The virtual address whose mapping we wish to access.
 *
 * Returns a pointer to the 64-bit page table entry in the recursive mapping region.
 *
 * This function constructs a recursive alias address that "dives" into the page table
 * hierarchy to yield the address of the desired PTE.
 */
uint64_t *get_pte_ptr(uint64_t virt_addr);

/**
 * vmm_map_recursive - Map a virtual address to a physical address.
 *
 * @virt_addr: The virtual address to map.
 * @phys_addr: The physical address to be mapped.
 * @flags:     Flags for the mapping (e.g., PAGE_PRESENT | PAGE_WRITE | PAGE_USER).
 *
 * This function uses the recursive mapping to locate the correct page table entry and
 * sets it up to map the provided physical address.
 */
void vmm_map_recursive(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags);

/**
 * vmm_unmap_recursive - Unmap a virtual address.
 *
 * @virt_addr: The virtual address to unmap.
 *
 * This function clears the page table entry corresponding to the virtual address.
 */
void vmm_unmap_recursive(uint64_t virt_addr);

#ifdef __cplusplus
}
#endif

#endif /* VMM_H */
