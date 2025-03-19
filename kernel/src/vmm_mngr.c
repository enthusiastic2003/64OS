#include "vmm_mngr.h"
#include "limine_requests.h"  // for hhdm_request
#include "pmm_mngr.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/* Zero out one page of memory. */
static void zero_page(void *addr) {
    uint8_t *p = (uint8_t *)addr;
    for (size_t i = 0; i < PAGE_SIZE; i++)
        p[i] = 0;
}

/**
 * get_pte_ptr - Get pointer to the page table entry (PTE) for a given virtual address.
 *
 * This function extracts the four indices (PML4, PDPT, PD, PT) from the virtual
 * address and constructs a recursive mapping address that “dives” into the paging
 * hierarchy. The final offset (multiplied by sizeof(uint64_t)) selects the exact PTE.
 */
uint64_t *get_pte_ptr(virt_addr_t virt_addr) {
    uint16_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint16_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint16_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint16_t pt_idx   = (virt_addr >> 12) & 0x1FF;
    
    return (uint64_t *)(RECURSIVE_BASE |
           ((uint64_t)pml4_idx << 30) |
           ((uint64_t)pdpt_idx << 21) |
           ((uint64_t)pd_idx   << 12) |
           (pt_idx * sizeof(uint64_t)));
}

/**
 * vmm_map_recursive - Map a virtual address to a physical address.
 *
 * Walks the paging hierarchy for the given virtual address. For any missing
 * intermediate table (PDPT, PD, or PT), allocates a new page, zeroes it, and
 * installs it with PAGE_PRESENT and PAGE_WRITE. Finally, sets the page table entry
 * for the virtual address to map to the provided physical address along with the
 * given flags.
 */
void vmm_map_recursive(virt_addr_t virt_addr, phys_addr_t phys_addr, uint64_t flags) {
    /* Extract indices for each level */
    uint16_t pml4_idx = (virt_addr >> 39) & 0x1FF;
    uint16_t pdpt_idx = (virt_addr >> 30) & 0x1FF;
    uint16_t pd_idx   = (virt_addr >> 21) & 0x1FF;
    uint16_t pt_idx   = (virt_addr >> 12) & 0x1FF;

    /* The recursive mapping makes the current PML4 available at RECURSIVE_BASE */
    uint64_t *pml4 = (uint64_t *)RECURSIVE_BASE;

    /* Ensure the PDPT exists: if not, allocate one */
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        phys_addr_t new_pdpt_phys = pmm_alloc();
        uint64_t *new_pdpt = (uint64_t *)(HHDM_OFFSET + new_pdpt_phys);
        zero_page(new_pdpt);
        pml4[pml4_idx] = new_pdpt_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    /* Access the PDPT table using recursive mapping */
    uint64_t *pdpt = (uint64_t *)(RECURSIVE_BASE | ((uint64_t)pml4_idx << 30));

    /* Ensure the PD exists */
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        phys_addr_t new_pd_phys = pmm_alloc();
        uint64_t *new_pd = (uint64_t *)(HHDM_OFFSET + new_pd_phys);
        zero_page(new_pd);
        pdpt[pdpt_idx] = new_pd_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    /* Access the PD table using recursive mapping */
    uint64_t *pd = (uint64_t *)(RECURSIVE_BASE | ((uint64_t)pml4_idx << 30) |
                                         ((uint64_t)pdpt_idx << 21));

    /* Ensure the PT exists */
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        phys_addr_t new_pt_phys = pmm_alloc();
        uint64_t *new_pt = (uint64_t *)(HHDM_OFFSET + new_pt_phys);
        zero_page(new_pt);
        pd[pd_idx] = new_pt_phys | PAGE_PRESENT | PAGE_WRITE;
    }
    /* Access the PT table using recursive mapping */
    uint64_t *pt = (uint64_t *)(RECURSIVE_BASE | ((uint64_t)pml4_idx << 30) |
                                         ((uint64_t)pdpt_idx << 21) |
                                         ((uint64_t)pd_idx   << 12));

    /* Set the page table entry: physical address with given flags, plus present bit */
    pt[pt_idx] = phys_addr | flags | PAGE_PRESENT;

    /* Invalidate the TLB for the virtual address */
    asm volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
}

/**
 * vmm_unmap_recursive - Unmap a virtual address.
 *
 * Retrieves the page table entry using get_pte_ptr() and clears it. It then flushes
 * the TLB entry for that virtual address.
 */
void vmm_unmap_recursive(virt_addr_t virt_addr) {
    uint64_t *pte = get_pte_ptr(virt_addr);
    *pte = 0;
    asm volatile ("invlpg (%0)" : : "r" (virt_addr) : "memory");
}
