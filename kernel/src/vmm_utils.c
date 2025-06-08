//
// Created by sirjanh on 6/7/25.
//
#include "pmm_mngr.h"
#include "vmm_mngr.h"
#include "vmm_utils.h"

#include <string.h>

#include "text_renderer.h"

phys_addr_t create_page_table() {
    phys_addr_t new_page_table = pmm_alloc();
    return new_page_table;
}

void free_page_table(phys_addr_t pt) {
    pmm_free(pt);
}

// Access PML4 table (no indices needed)
pml4_t* get_pml4_virtual_address(void) {
    uintptr_t va =
        (511UL << 39) |
        (511UL << 30) |
        (511UL << 21) |
        (511UL << 12);
    return (pml4_t*)va;
}

// Access PDPT table at given pdpt_index
pdpt_t* get_pdpt_virtual_address(int pdpt_index) {
    if (pdpt_index < 0 || pdpt_index > 511) return NULL;
    uintptr_t va =
        (511UL << 39) |
        (511UL << 30) |
        (511UL << 21) |
        ((uint64_t)pdpt_index << 12);

    return (pdpt_t*)va;
}

// Access PD table at given pdpt_index, pd_index
pd_t* get_pd_virtual_address(int pdpt_index, int pd_index) {
    if (pdpt_index < 0 || pdpt_index > 511 || pd_index < 0 || pd_index > 511) return NULL;
    uintptr_t va =
        (511UL << 39) |
        (511UL << 30) |
        ((uint64_t)pdpt_index << 21) |
        ((uint64_t)pd_index  << 12);
    return (pd_t*)va;
}

// Access PT table at given pdpt_index, pd_index, pt_index
pt_t* get_pt_virtual_address(int pdpt_index, int pd_index, int pt_index) {
    if (pdpt_index < 0 || pdpt_index > 511 || pd_index < 0 || pd_index > 511 || pt_index < 0 || pt_index > 511) return NULL;
    uintptr_t va =
        (511UL << 39) |
        ((uint64_t)pdpt_index << 30) |
        ((uint64_t)pd_index << 21) |
        ((uint64_t)pt_index << 12);
    return (pt_t*)va;
}

int map_page(virt_addr_t va, phys_addr_t pa, uint64_t flags) {
    int pml4_idx = (va >> 39) & 0x1FF;
    int pdpt_idx = (va >> 30) & 0x1FF;
    int pd_idx   = (va >> 21) & 0x1FF;
    int pt_idx   = (va >> 12) & 0x1FF;

    pml4_t* pml4 = get_pml4_virtual_address();
    if (!pml4) {
        kprintf("Failed to get PML4 virtual address\n");
        return -1;
    }

    // Ensure PDPT exists
    if (!(pml4->entries[pml4_idx].present)) {
        phys_addr_t new_pdpt = create_page_table();
        if (!new_pdpt) {
            kprintf("Failed to create new PDPT\n");
            return -1;
        }
        pml4->entries[pml4_idx].phys_addr = new_pdpt >> 12;
        pml4->entries[pml4_idx].present = 1;
        pml4->entries[pml4_idx].rw = 1;
        // Set other flags if needed (e.g., user/supervisor)
    }

    pdpt_t* pdpt = get_pdpt_virtual_address(pml4_idx);
    if (!pdpt) {
        kprintf("Failed to get PDPT virtual address\n");
        return -1;
    }

    // Ensure PD exists
    if (!(pdpt->entries[pdpt_idx].present)) {
        phys_addr_t new_pd = create_page_table();
        if (!new_pd) {
            kprintf("Failed to create new PD\n");
            return -1;
        }
        pdpt->entries[pdpt_idx].phys_addr = new_pd >> 12;
        pdpt->entries[pdpt_idx].present = 1;
        pdpt->entries[pdpt_idx].rw = 1;
    }

    pd_t* pd = get_pd_virtual_address(pml4_idx, pdpt_idx);
    if (!pd) {
        kprintf("Failed to get PD virtual address\n");
        return -1;
    }

    // Ensure PT exists
    if (!(pd->entries[pd_idx].present)) {
        phys_addr_t new_pt = create_page_table();
        if (!new_pt) {
            kprintf("Failed to create new PT\n");
            return -1;
        }
        pd->entries[pd_idx].phys_addr = new_pt >> 12;
        pd->entries[pd_idx].present = 1;
        pd->entries[pd_idx].rw = 1;
    }

    pt_t* pt = get_pt_virtual_address(pml4_idx, pdpt_idx, pd_idx);
    if (!pt) {
        kprintf("Failed to get PT virtual address\n");
        return -1;
    }

    // Check if the page is already mapped
    if (pt->entries[pt_idx].present) {
        kprintf("Warning: Page at VA %p is already mapped\n", (void*)va);
        // You might want to return an error here, or allow remapping
        // For now, we'll allow remapping
    }

    // Map the page
    pt->entries[pt_idx].phys_addr = pa >> 12;
    pt->entries[pt_idx].present = 1;
    pt->entries[pt_idx].rw = (flags & PAGE_RW) ? 1 : 0;
    pt->entries[pt_idx].user = (flags & PAGE_USER) ? 1 : 0;
    // Set other flags accordingly from 'flags'

    flush_tlb_single(va);

    return 0; // Success
}

void unmap_page(virt_addr_t va) {
    int pml4_idx = (va >> 39) & 0x1FF;
    int pdpt_idx = (va >> 30) & 0x1FF;
    int pd_idx   = (va >> 21) & 0x1FF;
    int pt_idx   = (va >> 12) & 0x1FF;

    pt_t* pt = get_pt_virtual_address(pml4_idx, pdpt_idx, pd_idx);
    if (!pt) return; // No page table present

    if (!pt->entries[pt_idx].present) return; // Already unmapped

    // Clear the entry
    pt->entries[pt_idx].present = 0;
    pt->entries[pt_idx].phys_addr = 0;
    pt->entries[pt_idx].rw = 0;
    pt->entries[pt_idx].user = 0;
    // Clear other flags if needed

    flush_tlb_single(va);
}

phys_addr_t get_physical_address(virt_addr_t va) {
    int pml4_idx = (va >> 39) & 0x1FF;
    int pdpt_idx = (va >> 30) & 0x1FF;
    int pd_idx   = (va >> 21) & 0x1FF;
    int pt_idx   = (va >> 12) & 0x1FF;
    uint64_t offset = va & 0xFFF;

    pml4_t* pml4 = get_pml4_virtual_address();
    if (!pml4->entries[pml4_idx].present)
        return 0;  // Not mapped

    pdpt_t* pdpt = get_pdpt_virtual_address(pml4_idx);
    if (!pdpt || !pdpt->entries[pdpt_idx].present)
        return 0;

    pd_t* pd = get_pd_virtual_address(pml4_idx, pdpt_idx);
    if (!pd || !pd->entries[pd_idx].present)
        return 0;

    pt_t* pt = get_pt_virtual_address(pml4_idx, pdpt_idx, pd_idx);
    if (!pt || !pt->entries[pt_idx].present)
        return 0;

    // Compose the physical address: frame base + offset
    phys_addr_t frame_addr = pt->entries[pt_idx].phys_addr << 12;
    return frame_addr + offset;
}

void set_page_flags(virt_addr_t va, uint64_t flags) {
    int pml4_idx = (va >> 39) & 0x1FF;
    int pdpt_idx = (va >> 30) & 0x1FF;
    int pd_idx   = (va >> 21) & 0x1FF;
    int pt_idx   = (va >> 12) & 0x1FF;

    pt_t* pt = get_pt_virtual_address(pml4_idx, pdpt_idx, pd_idx);
    if (!pt) return;

    if (!pt->entries[pt_idx].present) return;

    // Set flags based on your page table entry layout
    pt->entries[pt_idx].rw = (flags & PAGE_RW) ? 1 : 0;
    pt->entries[pt_idx].user = (flags & PAGE_USER) ? 1 : 0;
    // Add more flag setters if you have (e.g., NX, cache disable)

    flush_tlb_single(va);
}

void load_cr3(phys_addr_t pml4_addr) {
    __asm__ volatile ("mov %0, %%cr3" :: "r"(pml4_addr) : "memory");
}

void flush_tlb() {
    uintptr_t cr3_val;
    __asm__ volatile ("mov %%cr3, %0" : "=r" (cr3_val));
    __asm__ volatile ("mov %0, %%cr3" :: "r" (cr3_val) : "memory");
}

void flush_tlb_single(virt_addr_t va) {
    __asm__ volatile ("invlpg (%0)" :: "r"(va) : "memory");
}

int is_page_present(virt_addr_t va) {
    int pml4_idx = (va >> 39) & 0x1FF;
    int pdpt_idx = (va >> 30) & 0x1FF;
    int pd_idx   = (va >> 21) & 0x1FF;
    int pt_idx   = (va >> 12) & 0x1FF;

    pml4_t* pml4 = get_pml4_virtual_address();
    if (!pml4->entries[pml4_idx].present) return 0;

    pdpt_t* pdpt = get_pdpt_virtual_address(pml4_idx);
    if (!pdpt || !pdpt->entries[pdpt_idx].present) return 0;

    pd_t* pd = get_pd_virtual_address(pml4_idx, pdpt_idx);
    if (!pd || !pd->entries[pd_idx].present) return 0;

    pt_t* pt = get_pt_virtual_address(pml4_idx, pdpt_idx, pd_idx);
    if (!pt || !pt->entries[pt_idx].present) return 0;

    return 1;
}








