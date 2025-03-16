#pragma once
#include <stdint.h>
#include <stdbool.h>

// Page table entry flags (x86-64)
#define VMM_PRESENT      (1 << 0)
#define VMM_WRITABLE     (1 << 1)
#define VMM_USER         (1 << 2)
#define VMM_NX           (1 << 63) // Execute-disable

// Convert virtual to physical using HHDM
#define HHDM_OFFSET      hhdm_request.response->offset
#define VIRT_TO_PHYS(v)  ((uint64_t)(v) - HHDM_OFFSET)
#define PHYS_TO_VIRT(p)  ((uint64_t)(p) + HHDM_OFFSET)

// Initialize VMM (uses Limine's PML4)
void vmm_init();

// Map a virtual address to a physical address
void vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);

// Unmap a virtual address
void vmm_unmap_page(uint64_t virt);

// Get the physical address of a virtual page (returns 0 if unmapped)
uint64_t vmm_virt_to_phys(uint64_t virt);