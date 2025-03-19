#include "vmm_mngr.h"
#include "pmm_mngr.h"
#include "limine_requests.h"
#include "vmm_mngr_utils.h"
#include "text_renderer.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Assume virt_addr_t and phys_addr_t are defined appropriately
// and that PAGE_SIZE, RECURSIVE_BASE, and PAGE_PRESENT, etc., are defined

/* A simple structure to hold mapping information */

/**
 * vmm_map_range - Map a contiguous range of virtual addresses to physical addresses.
 *
 * @start:     The starting virtual address.
 * @size:      The size of the region in bytes.
 * @phys_start:The starting physical address.
 * @flags:     Flags for each mapping (e.g., PAGE_PRESENT | PAGE_WRITE | PAGE_USER).
 *
 * Maps each page in the region by calling vmm_map_recursive.
 */
void vmm_map_range(virt_addr_t start, size_t size, phys_addr_t phys_start, uint64_t flags) {
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        vmm_map_recursive(start + i * PAGE_SIZE, phys_start + i * PAGE_SIZE, flags);
    }
}

/**
 * vmm_unmap_range - Unmap a contiguous range of virtual addresses.
 *
 * @start: The starting virtual address.
 * @size:  The size of the region in bytes.
 *
 * Unmaps each page in the region by calling vmm_unmap_recursive.
 */
void vmm_unmap_range(virt_addr_t start, size_t size) {
    size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        vmm_unmap_recursive(start + i * PAGE_SIZE);
    }
}

/**
 * vmm_query_mapping - Retrieve mapping details for a virtual address.
 *
 * @virt_addr: The virtual address to query.
 *
 * Returns a mapping_info_t structure containing the physical address and flags
 * of the mapping. If the page is not present, both fields will be zero.
 */
mapping_info_t vmm_query_mapping(virt_addr_t virt_addr) {
    mapping_info_t info = {0, 0};
    uint64_t *pte = get_pte_ptr(virt_addr);
    if (*pte & PAGE_PRESENT) {
        // Physical address is stored in the upper bits; lower 12 bits are flags.
        info.phys_addr = *pte & ~((uint64_t)0xFFF);
        info.flags = *pte & 0xFFF;
    }
    return info;
}

/**
 * vmm_change_flags - Modify the flags for an existing mapping.
 *
 * @virt_addr:  The virtual address whose mapping is to be modified.
 * @new_flags:  The new flag bits (besides PAGE_PRESENT) to apply.
 *
 * This function preserves the physical address stored in the PTE while replacing
 * the flag bits.
 */
void vmm_change_flags(virt_addr_t virt_addr, uint64_t new_flags) {
    uint64_t *pte = get_pte_ptr(virt_addr);
    if (*pte & PAGE_PRESENT) {
        phys_addr_t phys_addr = *pte & ~((uint64_t)0xFFF);
        *pte = phys_addr | new_flags | PAGE_PRESENT;
        asm volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
    }
}

/**
 * vmm_dump_page_tables - Dump the current state of the page tables.
 *
 * For debugging purposes, this function iterates through the PML4 and its lower
 * levels (PDPT, PD, PT) via the recursive mapping and prints out non-empty entries.
 */
void vmm_dump_page_tables(void) {
    uint64_t *pml4 = (uint64_t *)RECURSIVE_BASE;
    for (int pml4_idx = 0; pml4_idx < 512; pml4_idx++) {
        if (pml4[pml4_idx] & PAGE_PRESENT) {
            kprintf("PML4[%d] = 0x%lx\n", pml4_idx, pml4[pml4_idx]);
            // Skip the recursive mapping entry to avoid infinite recursion.
            if (pml4_idx == RECURSIVE_INDEX)
                continue;
            uint64_t *pdpt = (uint64_t *)(RECURSIVE_BASE | ((uint64_t)pml4_idx << 30));
            for (int pdpt_idx = 0; pdpt_idx < 512; pdpt_idx++) {
                if (pdpt[pdpt_idx] & PAGE_PRESENT) {
                    kprintf("  PDPT[%d] = 0x%lx\n", pdpt_idx, pdpt[pdpt_idx]);
                    uint64_t *pd = (uint64_t *)(RECURSIVE_BASE | ((uint64_t)pml4_idx << 30) |
                                                    ((uint64_t)pdpt_idx << 21));
                    for (int pd_idx = 0; pd_idx < 512; pd_idx++) {
                        if (pd[pd_idx] & PAGE_PRESENT) {
                            kprintf("    PD[%d] = 0x%lx\n", pd_idx, pd[pd_idx]);
                            uint64_t *pt = (uint64_t *)(RECURSIVE_BASE | ((uint64_t)pml4_idx << 30) |
                                                            ((uint64_t)pdpt_idx << 21) |
                                                            ((uint64_t)pd_idx << 12));
                            for (int pt_idx = 0; pt_idx < 512; pt_idx++) {
                                if (pt[pt_idx] & PAGE_PRESENT) {
                                    kprintf("      PT[%d] = 0x%lx\n", pt_idx, pt[pt_idx]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
