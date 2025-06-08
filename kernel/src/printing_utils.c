//
// Created by sirjanh on 6/7/25.
//

#include "text_renderer.h"
#include "vmm_mngr.h"

#define ENTRIES_PER_TABLE 512

// Forward declarations
void print_page_table_level(void* table_virt_addr, int level, uint64_t hhdm_base);

void print_page_table(uint64_t virt_pml4_addr, uint64_t hhdm_base) {
    kprintf("Printing page tables starting at PML4 @ virt 0x%lx\n", virt_pml4_addr);
    print_page_table_level((void*)virt_pml4_addr, 4, hhdm_base);
}

void print_indent(int level) {
    for (int i = 0; i < (4 - level) * 4; i++) {
        kprintf(" ");
    }
}

void print_page_table_level(void* table_virt_addr, int level, uint64_t hhdm_base) {
    page_table_entry_t* entries = (page_table_entry_t*)table_virt_addr;
    const char* level_names[] = { "PT", "PD", "PDPT", "PML4" };
    kprintf("%s table @ virt %p\n", level_names[level - 1], table_virt_addr);

    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        page_table_entry_t entry = entries[i];

        if (!entry.present) {
            // Optionally print non-present entries:
            // print_indent(level);
            // kprintf("[%3d] not present\n", i);
            continue;
        }

        print_indent(level);
        kprintf("[%d] present=1 rw=%d phys=0x%lx\n", i, entry.rw, entry.phys_addr << 12);

        if (level > 1) {
            // Recursively print next level table
            void* next_level_virt = (void*)((entry.phys_addr << 12) + hhdm_base);
            print_page_table_level(next_level_virt, level - 1, hhdm_base);
        }
    }
}
