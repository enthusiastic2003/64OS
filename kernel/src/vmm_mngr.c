//
// Created by sirjanh on 6/7/25.
// Fixed version with corrected buddy allocator logic
//

#include "vmm_mngr.h"

#include "limine_requests.h"
#include "pmm_mngr.h"
#include "string.h"
#include "text_renderer.h"
#include "vmm_utils.h"

#define MAX_ORDER 16  // Supports up to 2^16 * page_size allocations
#define PAGE_SIZE 4096
#define VM_REGION_START 0xFFFF800000000000ULL
#define VM_REGION_SIZE (1ULL << 32) // 4 GiB for virtual memory buddy allocator

static inline void write_cr3(uint64_t phys_addr) {
    __asm__ volatile (
        "mov %0, %%cr3"
        :
        : "r" (phys_addr)
        : "memory"
    );
}

void remap_kernel_pages() {
    // Get the location of the current PML4 physical address (CR3)
    uint64_t old_cr3 = read_cr3();

    kprintf("\nOld cr3: %x\n", old_cr3);

    uint64_t hhdm_base = hhdm_request.response->offset;

    // Map old CR3 physical address into virtual memory via HHDM
    virt_addr_t virt_paging_base = old_cr3 + hhdm_base;

    // Allocate a new PML4 page
    uint64_t phys_new_pml4 = pmm_alloc();
    virt_addr_t virt_new_pml4 = phys_new_pml4 + hhdm_base;

    // Cast pointers for old and new PML4
    pml4_t* old_pml4 = (pml4_t*)virt_paging_base;
    pml4_t* new_pml4 = (pml4_t*)virt_new_pml4;

    // Zero out the new PML4 page before use
    memset(new_pml4, 0, PAGE_SIZE);

    int total_pages_acquired = 1;

    // Check if index 511 is already in use for recursive mapping
    bool recursive_mapping_exists = old_pml4->entries[511].present;

    // Setup recursive mapping entry in new PML4 at index 511 (if not conflicting)
    if (!recursive_mapping_exists) {
        new_pml4->entries[511].present = 1;
        new_pml4->entries[511].rw = 1;
        new_pml4->entries[511].user = 0;  // supervisor only
        new_pml4->entries[511].phys_addr = phys_new_pml4 >> 12;
    }

    // Walk through all PML4 entries
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        page_table_entry_t old_pdpt_entry = old_pml4->entries[i];
        if (!old_pdpt_entry.present) continue;

        // Check for 1GB pages (PS bit set in PDPT entry)
        if (old_pdpt_entry.ps) {
            // This is a 1GB page, copy directly
            new_pml4->entries[i] = old_pdpt_entry;
            continue;
        }

        // Get virtual address of old PDPT
        pdpt_t* old_pdpt = (pdpt_t*)((old_pdpt_entry.phys_addr << 12) + hhdm_base);

        // Allocate and zero new PDPT
        uint64_t phys_new_pdpt = pmm_alloc();
        total_pages_acquired++;
        virt_addr_t virt_new_pdpt = phys_new_pdpt + hhdm_base;
        pdpt_t* new_pdpt = (pdpt_t*)virt_new_pdpt;
        memset(new_pdpt, 0, PAGE_SIZE);

        // Set the PDPT entry in the new PML4, copying flags from old entry
        new_pml4->entries[i] = old_pdpt_entry;
        new_pml4->entries[i].phys_addr = phys_new_pdpt >> 12;

        // Iterate over PD entries in old PDPT
        for (int j = 0; j < ENTRIES_PER_TABLE; j++) {
            page_table_entry_t old_pd_entry = old_pdpt->entries[j];
            if (!old_pd_entry.present) continue;

            // Check for 2MB pages (PS bit set in PD entry)
            if (old_pd_entry.ps) {
                // This is a 2MB page, copy directly
                new_pdpt->entries[j] = old_pd_entry;
                continue;
            }

            pd_t* old_pd = (pd_t*)((old_pd_entry.phys_addr << 12) + hhdm_base);

            // Allocate and zero new PD
            uint64_t phys_new_pd = pmm_alloc();
            total_pages_acquired++;
            pd_t* new_pd = (pd_t*)(phys_new_pd + hhdm_base);
            memset(new_pd, 0, PAGE_SIZE);

            // Set PD entry in new PDPT, copying flags
            new_pdpt->entries[j] = old_pd_entry;
            new_pdpt->entries[j].phys_addr = phys_new_pd >> 12;

            // Iterate over PT entries in old PD
            for (int k = 0; k < ENTRIES_PER_TABLE; k++) {
                page_table_entry_t old_pt_entry = old_pd->entries[k];
                if (!old_pt_entry.present) continue;

                pt_t* old_pt = (pt_t*)((old_pt_entry.phys_addr << 12) + hhdm_base);

                // Allocate and zero new PT
                uint64_t phys_new_pt = pmm_alloc();
                total_pages_acquired++;
                pt_t* new_pt = (pt_t*)(phys_new_pt + hhdm_base);
                memset(new_pt, 0, PAGE_SIZE);

                // Set PT entry in new PD, copying flags
                new_pd->entries[k] = old_pt_entry;
                new_pd->entries[k].phys_addr = phys_new_pt >> 12;

                // Copy all page table entries from old PT to new PT
                memcpy(new_pt, old_pt, sizeof(pt_t));
            }
        }
    }

    // Handle recursive mapping if it existed in the original
    if (recursive_mapping_exists) {
        // If index 511 was already used, we need to copy that mapping too
        // and then update it to point to our new PML4 for recursive access
        page_table_entry_t old_recursive = old_pml4->entries[511];
        new_pml4->entries[511] = old_recursive;

        // Update the recursive mapping to point to the new PML4
        new_pml4->entries[511].phys_addr = phys_new_pml4 >> 12;
    }

    // Switch to the new page tables:
    write_cr3(phys_new_pml4);
    kprintf("\nTotal pages acquired: %d\n", total_pages_acquired);
    kprintf("\nVoila! New Paging Structure Enabled!!!\n");
    kprintf("\nNew CR3: %x\n", read_cr3());
}

// Fixed buddy allocator implementation
typedef struct BuddyBlock {
    struct BuddyBlock* next;
} BuddyBlock;

static BuddyBlock* free_lists[MAX_ORDER];
static uintptr_t base = VM_REGION_START;
static size_t num_pages = VM_REGION_SIZE / PAGE_SIZE;
static uint8_t* block_orders; // One byte per page, holds the order if free or 255 if allocated

void vm_buddy_allocator_init() {
    // Calculate how much memory we need for the block_orders array
    size_t array_size = num_pages * sizeof(uint8_t);
    size_t pages_needed = (array_size + PAGE_SIZE - 1) / PAGE_SIZE;

    kprintf("VM Buddy allocator needs %zu pages for %zu total pages metadata\n", pages_needed, num_pages);

    uint64_t hhdm_base = hhdm_request.response->offset;
    kprintf("HHDM base: %p\n", (void*)hhdm_base);

    if (pages_needed == 1) {
        // Simple case: only need one page
        phys_addr_t phys_page = pmm_alloc();
        if (!phys_page) {
            kprintf("Failed to allocate memory for buddy allocator metadata\n");
            return;
        }
        kprintf("Allocated physical page: %p for metadata\n", (void*)phys_page);

        // Use HHDM to access the physical page
        virt_addr_t virt_page = phys_page + hhdm_base;
        kprintf("Mapped to virtual address: %p\n", (void*)virt_page);

        block_orders = (uint8_t*)virt_page;

        // Test if we can actually write to this memory
        kprintf("Testing memory access...\n");
        block_orders[0] = 42;  // Test write
        if (block_orders[0] != 42) {
            kprintf("Memory test failed - cannot write to HHDM mapped memory\n");
            return;
        }
        kprintf("Memory test passed\n");

    } else {
        // Multiple pages needed: use virtual mapping approach
        kprintf("Multiple pages needed, using virtual mapping\n");

        // Use a fixed virtual address after your VM region for metadata
        uintptr_t metadata_vaddr = VM_REGION_START + VM_REGION_SIZE;
        kprintf("Using virtual address %p for metadata\n", (void*)metadata_vaddr);

        // Allocate and map each page
        for (size_t i = 0; i < pages_needed; i++) {

            phys_addr_t phys_page = pmm_alloc();
            if (!phys_page) {
                kprintf("Failed to allocate physical page %zu/%zu for metadata\n", i + 1, pages_needed);
                // Clean up previously allocated and mapped pages
                for (size_t j = 0; j < i; j++) {
                    virt_addr_t cleanup_va = metadata_vaddr + j * PAGE_SIZE;
                    phys_addr_t cleanup_pa = get_physical_address(cleanup_va);
                    if (cleanup_pa) {
                        pmm_free(cleanup_pa);
                    }
                    unmap_page(cleanup_va);
                }
                return;
            }

            kprintf("Mapping page %zu: PA %p -> VA %p\n", i, (void*)phys_page, (void*)(metadata_vaddr + i * PAGE_SIZE));

            // Map this physical page to virtual space
            if (map_page(metadata_vaddr + i * PAGE_SIZE, phys_page, PAGE_RW | PAGE_PRESENT) < 0) {
                kprintf("Failed to map metadata page %zu\n", i);
                pmm_free(phys_page);
                // Clean up previously allocated and mapped pages
                for (size_t j = 0; j < i; j++) {
                    virt_addr_t cleanup_va = metadata_vaddr + j * PAGE_SIZE;
                    phys_addr_t cleanup_pa = get_physical_address(cleanup_va);
                    if (cleanup_pa) {
                        pmm_free(cleanup_pa);
                    }
                    unmap_page(cleanup_va);
                }
                return;
            }
        }

        block_orders = (uint8_t*)metadata_vaddr;

        // Test memory access
        kprintf("Testing virtual mapped memory access...\n");
        block_orders[0] = 42;
        if (block_orders[0] != 42) {
            kprintf("Virtual memory test failed\n");
            return;
        }
        kprintf("Virtual memory test passed\n");
    }

    if (!block_orders) {
        kprintf("Failed to set up block_orders array\n");
        return;
    }

    kprintf("Initializing block_orders array at %p\n", (void*)block_orders);

    // Initialize all as allocated initially (255 means allocated/unavailable)
    for (size_t i = 0; i < num_pages; ++i) {
        block_orders[i] = 255;
    }

    kprintf("Initialized %zu entries in block_orders array\n", num_pages);

    // Initialize all free lists to NULL
    for (int i = 0; i < MAX_ORDER; i++) {
        free_lists[i] = NULL;
    }

    kprintf("Creating initial free blocks...\n");

    // Create initial free blocks - start with largest possible blocks
    size_t remaining_pages = num_pages;
    size_t current_page = 0;

    while (remaining_pages > 0) {
        // Find the largest order that fits
        int order = MAX_ORDER - 1;
        size_t block_size = 1ULL << order;

        while (block_size > remaining_pages && order > 0) {
            order--;
            block_size = 1ULL << order;
        }

        // Create a free block of this order
        uintptr_t block_addr = base + current_page * PAGE_SIZE;
        kprintf("Creating free block: addr=%p, order=%d, size=%zu pages\n",
               (void*)block_addr, order, block_size);

        add_block_to_list(block_addr, order);

        current_page += block_size;
        remaining_pages -= block_size;
    }

    kprintf("VM Buddy allocator initialized successfully with %zu pages\n", num_pages);
}

static size_t addr_to_page(uintptr_t addr) {
    return (addr - base) / PAGE_SIZE;
}

static uintptr_t page_to_addr(size_t page) {
    return base + page * PAGE_SIZE;
}

static void add_block_to_list(uintptr_t addr, int order) {
    size_t page = addr_to_page(addr);
    if (page >= num_pages) return; // Safety check

    block_orders[page] = order;
    BuddyBlock* block = (BuddyBlock*)addr;
    block->next = free_lists[order];
    free_lists[order] = block;
}

static uintptr_t remove_block_from_list(int order) {
    BuddyBlock* block = free_lists[order];
    if (!block) return 0;

    free_lists[order] = block->next;
    size_t page = addr_to_page((uintptr_t)block);
    if (page < num_pages) {
        block_orders[page] = 255; // Mark as allocated
    }
    return (uintptr_t)block;
}

void* vm_buddy_alloc(int order) {
    if (order >= MAX_ORDER || order < 0) {
        kprintf("Invalid order %d for buddy allocation\n", order);
        return NULL;
    }

    // Find a free block of sufficient size
    int current_order = order;
    while (current_order < MAX_ORDER && !free_lists[current_order]) {
        current_order++;
    }

    if (current_order == MAX_ORDER) {
        kprintf("No free blocks available for order %d\n", order);
        return NULL;
    }

    // Split larger blocks down to the required size
    while (current_order > order) {
        uintptr_t block = remove_block_from_list(current_order);
        if (!block) {
            kprintf("Failed to remove block from list at order %d\n", current_order);
            return NULL;
        }

        current_order--;
        size_t block_size = (1ULL << current_order) * PAGE_SIZE;
        uintptr_t buddy = block + block_size;

        // Add both halves back to the smaller order list
        add_block_to_list(block, current_order);
        add_block_to_list(buddy, current_order);
    }

    // Remove the final block from the free list
    uintptr_t addr = remove_block_from_list(order);
    if (!addr) {
        kprintf("Failed to allocate block of order %d\n", order);
        return NULL;
    }

    // Map physical pages for this virtual region
    size_t pages_to_map = 1ULL << order;
    for (size_t i = 0; i < pages_to_map; ++i) {
        uintptr_t va = addr + i * PAGE_SIZE;
        phys_addr_t pa = pmm_alloc(); // Your PMM returns a single page
        if (!pa) {
            kprintf("Failed to allocate physical page %zu/%zu\n", i + 1, pages_to_map);
            // Clean up already mapped pages
            for (size_t j = 0; j < i; j++) {
                uintptr_t cleanup_va = addr + j * PAGE_SIZE;
                phys_addr_t cleanup_pa = get_physical_address(cleanup_va);
                if (cleanup_pa) {
                    pmm_free(cleanup_pa);
                }
                unmap_page(cleanup_va);
            }
            // Return the virtual block to free list
            add_block_to_list(addr, order);
            return NULL;
        }

        map_page(va, pa, PAGE_RW | PAGE_PRESENT);
        // Since your map_page doesn't return an error code, we assume it succeeded
        // You could add additional
    }

    kprintf("Successfully allocated %zu pages at virtual address %p\n", pages_to_map, (void*)addr);
    return (void*)addr;
}

void vm_buddy_free(void* ptr, int order) {
    if (!ptr || order < 0 || order >= MAX_ORDER) {
        kprintf("Invalid parameters for vm_buddy_free: ptr=%p, order=%d\n", ptr, order);
        return;
    }

    uintptr_t addr = (uintptr_t)ptr;
    size_t page = addr_to_page(addr);

    if (page >= num_pages) {
        kprintf("Invalid address %p for vm_buddy_free\n", ptr);
        return;
    }

    // Unmap and free physical pages
    size_t pages_to_free = 1ULL << order;
    for (size_t i = 0; i < pages_to_free; ++i) {
        uintptr_t va = addr + i * PAGE_SIZE;
        phys_addr_t pa = get_physical_address(va); // You'll need this function
        if (pa) {
            pmm_free(pa);
        }
        unmap_page(va);
    }

    // Coalesce with buddy blocks
    while (order < MAX_ORDER - 1) {
        size_t buddy_page = page ^ (1ULL << order);
        if (buddy_page >= num_pages || block_orders[buddy_page] != order) {
            break; // Buddy is not free or doesn't exist
        }

        uintptr_t buddy_addr = page_to_addr(buddy_page);

        // Remove buddy from free list
        BuddyBlock** prev = &free_lists[order];
        while (*prev && (uintptr_t)(*prev) != buddy_addr) {
            prev = &((*prev)->next);
        }
        if (*prev) {
            *prev = (*prev)->next;
            block_orders[buddy_page] = 255; // Mark as allocated temporarily
        } else {
            break; // Buddy not found in free list
        }

        // Merge with buddy - ensure we use the lower address
        if (buddy_page < page) {
            page = buddy_page;
            addr = buddy_addr;
        }
        order++;
    }

    // Add the (possibly coalesced) block back to free list
    add_block_to_list(addr, order);
    kprintf("Freed block at %p with final order %d\n", ptr, order);
}

// High-level allocation functions
void* vm_alloc_pages(size_t num_pages) {
    if (num_pages == 0) return NULL;

    int order = 0;
    while ((1ULL << order) < num_pages) order++;

    return vm_buddy_alloc(order);
}

void vm_free_pages(void* addr, size_t num_pages) {
    if (!addr || num_pages == 0) return;

    int order = 0;
    while ((1ULL << order) < num_pages) order++;

    vm_buddy_free(addr, order);
}