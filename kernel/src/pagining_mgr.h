#include <stdint.h>
#include <stddef.h>
#include "text_renderer.h"

#define PML4_SHIFT 39
#define PDP_SHIFT 30
#define PD_SHIFT 21
#define PT_SHIFT 12

#define PML4_INDEX(vaddr) (((vaddr) >> PML4_SHIFT) & 0x1FF)
#define PDP_INDEX(vaddr)  (((vaddr) >> PDP_SHIFT) & 0x1FF)
#define PD_INDEX(vaddr)   (((vaddr) >> PD_SHIFT) & 0x1FF)
#define PT_INDEX(vaddr)   (((vaddr) >> PT_SHIFT) & 0x1FF)

#define PAGE_PRESENT  (1 << 0)
#define PAGE_RW       (1 << 1)
#define PAGE_USER     (1 << 2)

static inline uint64_t read_cr3() {
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

// Assuming identity mapping or a known KERNEL_BASE
#define KERNEL_VIRTUAL_BASE 0xFFFF800000000000

void print_page_mappings() {
    uint64_t cr3 = read_cr3(); 
    uint64_t *pml4 = (uint64_t *)(cr3 + KERNEL_VIRTUAL_BASE);

    for (size_t i = 0; i < 512; i++) {
        if (!(pml4[i] & PAGE_PRESENT)) continue;
        uint64_t *pdp = (uint64_t *)((pml4[i] & ~0xFFF) + KERNEL_VIRTUAL_BASE);

        for (size_t j = 0; j < 512; j++) {
            if (!(pdp[j] & PAGE_PRESENT)) continue;
            uint64_t *pd = (uint64_t *)((pdp[j] & ~0xFFF) + KERNEL_VIRTUAL_BASE);

            for (size_t k = 0; k < 512; k++) {
                if (!(pd[k] & PAGE_PRESENT)) continue;
                uint64_t *pt = (uint64_t *)((pd[k] & ~0xFFF) + KERNEL_VIRTUAL_BASE);

                for (size_t l = 0; l < 512; l++) {
                    if (!(pt[l] & PAGE_PRESENT)) continue;
                    
                    uint64_t vaddr = ((uint64_t)i << PML4_SHIFT) |
                                     ((uint64_t)j << PDP_SHIFT) |
                                     ((uint64_t)k << PD_SHIFT) |
                                     ((uint64_t)l << PT_SHIFT);

                    uint64_t paddr = pt[l] & ~0xFFF;
                    kprintf("Virtual: 0x%lx -> Physical: 0x%lx\n", vaddr, paddr);
                }
            }
        }
    }
}