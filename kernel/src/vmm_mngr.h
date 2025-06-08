#ifndef VMM_MNGR_H
#define VMM_MNGR_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE           4096  // 4 KiB
#define PAGE_PRESENT        (1ULL << 0)
#define PAGE_RW             (1ULL << 1)
#define PAGE_USER           (1ULL << 2)
#define PAGE_PWT            (1ULL << 3)
#define PAGE_PCD            (1ULL << 4)
#define PAGE_ACCESSED       (1ULL << 5)
#define PAGE_DIRTY          (1ULL << 6)
#define PAGE_HUGE           (1ULL << 7)
#define PAGE_GLOBAL         (1ULL << 8)
#define PAGE_NO_EXECUTE     (1ULL << 63)

#define ADDR_MASK_4KB       0x000FFFFFFFFFF000ULL

#define ENTRIES_PER_TABLE   512

// Extract 9 bits at the specified position
#define EXTRACT_9BITS(value, shift) (((value) >> (shift)) & 0x1FF)

// Virtual address parts in x86_64 paging
#define VA_PML4_IDX(va) EXTRACT_9BITS((va), 39)
#define VA_PDPT_IDX(va) EXTRACT_9BITS((va), 30)
#define VA_PD_IDX(va)   EXTRACT_9BITS((va), 21)
#define VA_PT_IDX(va)   EXTRACT_9BITS((va), 12)
#define VA_OFFSET(va)   ((va) & 0xFFF)   // Lower 12 bits offset


// Common structure for a page table entry
typedef struct page_table_entry {
    uint64_t present        : 1;
    uint64_t rw             : 1;
    uint64_t user           : 1;
    uint64_t pwt            : 1;
    uint64_t pcd            : 1;
    uint64_t accessed       : 1;
    uint64_t dirty          : 1;  // For leaf pages, or reserved for non-leaf
    uint64_t ps             : 1;  // Page Size bit (PAT for 4KB pages)
    uint64_t global         : 1;  // Global bit (ignored for non-leaf)
    uint64_t ignored1       : 3;
    uint64_t phys_addr      : 40;   // bits 12–51 of physical address
    uint64_t ignored2       : 11;
    uint64_t no_execute     : 1;
} __attribute__((packed)) page_table_entry_t;

// Page tables for each level
typedef struct page_table {
    page_table_entry_t entries[ENTRIES_PER_TABLE];
} __attribute__((aligned(PAGE_SIZE))) page_table_t;

// Aliases for each level
typedef page_table_t pml4_t;
typedef page_table_t pdpt_t;
typedef page_table_t pd_t;
typedef page_table_t pt_t;
typedef uint64_t virt_addr_t ;

void remap_kernel_pages();
void vm_buddy_allocator_init();
void* vm_alloc_pages(size_t num_pages);
void vm_free_pages(void* addr, size_t num_pages);
void vm_buddy_allocator_init();
static size_t addr_to_page(uintptr_t addr);
static uintptr_t page_to_addr(size_t page);
static void add_block_to_list(uintptr_t addr, int order);

static uintptr_t remove_block_from_list(int order);
void* vm_buddy_alloc(int order);
void vm_buddy_free(void* ptr, int order);
void vm_free_pages(void* addr, size_t num_pages);
#endif // VMM_MNGR_H
