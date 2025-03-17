#pragma once

#include <stdint.h>
#include <stdbool.h>

// Page table entry flags (x86-64)
#define VMM_PRESENT      (1 << 0)  // Page is present
#define VMM_WRITABLE     (1 << 1)  // Page is writable
#define VMM_USER         (1 << 2)  // User-mode accessible
#define VMM_NX           (1ULL << 63) // Execute-disable

// HHDM offset (provided by Limine)
extern uint64_t HHDM_OFFSET;

// Convert virtual to physical using HHDM
#define VIRT_TO_PHYS(v)  ((uint64_t)(v) - HHDM_OFFSET)
#define PHYS_TO_VIRT(p)  ((uint64_t)(p) + HHDM_OFFSET)

/**
 * Initialize the Virtual Memory Manager.
 * Uses Limine's PML4 and sets up paging.
 */
void vmm_init();

/**
 * Maps a virtual address to a physical address with given flags.
 * @param virt Virtual address.
 * @param phys Physical address.
 * @param flags Page flags (e.g., VMM_PRESENT | VMM_WRITABLE).
 */
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

/**
 * Unmaps a virtual address, removing its mapping.
 * @param virt Virtual address to unmap.
 */
void vmm_unmap_page(uint64_t virt);

/**
 * Get the physical address of a virtual page.
 * @param virt Virtual address.
 * @return Physical address if mapped, 0 otherwise.
 */
uint64_t vmm_virt_to_phys(uint64_t virt);

/**
 * Get the flags of a virtual page.
 * @param virt Virtual address.
 * @return Flags if mapped, 0 otherwise.
 */
uint64_t vmm_get_flags(uint64_t virt);

/**
 * Check if a virtual page is mapped.
 * @param virt Virtual address.
 * @return True if mapped, false otherwise.
 */
bool vmm_is_mapped(uint64_t virt);

/**
 * Enables paging by loading the PML4 table and setting CR3.
 */
void vmm_enable_paging();

/**
 * preserve_limine_requests
 */
void preserve_limine_requests();