#ifndef PMM_MNGR_H

#define PMM_MNGR_H

#define PAGE_SIZE 4096

#include <stdint.h>
#include <stddef.h>
#include "limine.h"

#define PAGE_SIZE 4096

static inline uint64_t read_cr3() {
    uint64_t cr3;
    asm volatile ("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

typedef uint64_t phys_addr_t;



void pmm_init(struct limine_memmap_request memmap_request, struct limine_hhdm_request hhdm_request);
void pmm_free(uint64_t phys_addr);
uint64_t pmm_alloc();
void print_and_init_memmap(struct limine_memmap_request memmap_request, struct limine_hhdm_request hhdm_request);

uint64_t get_free_frame_count();
uint64_t get_used_frame_count();
uint64_t get_total_frame_count();
#endif