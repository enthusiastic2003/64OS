#include <stdint.h>
#include <stddef.h>
#include "text_renderer.h"
#include "limine.h" // Include the header file for limine_hhdm_request
#include "pmm_mngr.h"
#include "limine_requests.h"

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


// Assuming identity mapping or a known KERNEL_BASE
#define KERNEL_VIRTUAL_BASE 0xFFFF800000000000


// HHDM (Higher Half Direct Mapping) information





void inspect_page_tables() {
    uint64_t cr3 = read_cr3();
    uint64_t* pml4 = (uint64_t*)(cr3 & ~0xFFF);


    uint64_t hhdm_offset = hhdm_request.response->offset;
    kprintf("HHDM offset: 0x%lx\n", hhdm_offset);

    // Convert CR3 physical address to virtual
    pml4 = (uint64_t*)(hhdm_offset + (uint64_t)pml4);

    for (int i = 0; i < 512; i++) {

        if (i == 256) {
            kprintf("Skipping self-referential entry\n");
            continue;
        }

        if (!(pml4[i] & 1)) continue; // Skip non-present entries

        kprintf("PML4[%d] = 0x%lx\n", i, pml4[i]);

        uint64_t* pdpt = (uint64_t*)((pml4[i] & ~0xFFF) + hhdm_offset);
        for (int j = 0; j < 512; j++) {
            if (!(pdpt[j] & 1)) continue;

            kprintf("  PDPT[%d] = 0x%lx\n", j, pdpt[j]);

            if (pdpt[j] & (1 << 7)) { // 1GB page check
                uint64_t virt_addr = ((uint64_t)i << 39) | ((uint64_t)j << 30);
                kprintf("    [1GB PAGE] Virtual: 0x%lx\n", virt_addr);
                continue;
            }

            uint64_t* pd = (uint64_t*)((pdpt[j] & ~0xFFF) + hhdm_offset);
            for (int k = 0; k < 512; k++) {
                if (!(pd[k] & 1)) continue;

                if (pd[k] & (1 << 7)) { // 2MB page check
                    uint64_t virt_addr = ((uint64_t)i << 39) | ((uint64_t)j << 30) | ((uint64_t)k << 21);
                    kprintf("    [2MB PAGE] Virtual: 0x%lx\n", virt_addr);
                    continue;
                }

                uint64_t* pt = (uint64_t*)((pd[k] & ~0xFFF) + hhdm_offset);
                for (int l = 0; l < 512; l++) {
                    if (!(pt[l] & 1)) continue;

                    uint64_t virt_addr = ((uint64_t)i << 39) | ((uint64_t)j << 30) | ((uint64_t)k << 21) | ((uint64_t)l << 12);
                    kprintf("      [4KB PAGE] Virtual: 0x%lx\n", virt_addr);
                }
            }
        }
    }
}
