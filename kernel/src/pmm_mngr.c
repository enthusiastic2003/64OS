#include<stdint.h>
#include "pmm_mngr.h"
#include "limine.h"
#include "string.h"
#include "text_renderer.h"

struct limine_memmap_entry **memmap_entries;
uint64_t memmap_entry_count;
uint8_t *pmm_bitmap;
uint64_t bitmap_size;
uint64_t pmm_total_frames = 0;
uint64_t pmm_used_frames = 0;

// Physical memory bitmap (1 bit per 4KB frame)

uint64_t get_free_frame_count() {
    return pmm_total_frames - pmm_used_frames;
}

uint64_t get_used_frame_count() {
    return pmm_used_frames;
}

uint64_t get_total_frame_count() {
    return pmm_total_frames;
}


uint64_t pmm_alloc() {
    for (uint64_t i = 0; i < pmm_total_frames; i++) {
        if (!(pmm_bitmap[i / 8] & (1 << (i % 8)))) { // Frame is free
            pmm_bitmap[i / 8] |= (1 << (i % 8));     // Mark as used
            pmm_used_frames++;
            return i * PAGE_SIZE; // Return physical address
        }
    }
    return 0; // Out of memory
}

void pmm_free(uint64_t phys_addr) {
    uint64_t frame = phys_addr / PAGE_SIZE;
    pmm_bitmap[frame / 8] &= ~(1 << (frame % 8)); // Mark as free
    pmm_used_frames--;
}

// Initialize the PMM using Limine's memory map
void pmm_init(struct limine_memmap_request memmap_request, struct limine_hhdm_request hhdm_request) {
    struct limine_memmap_response *memmap = memmap_request.response;
    memmap_entries = memmap->entries;
    memmap_entry_count = memmap->entry_count;

    // Step 1: Calculate total memory and find the largest usable region for the bitmap
    uint64_t total_memory = 0;
    uint64_t largest_region_size = 0;
    uint64_t largest_region_base = 0;

    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            total_memory += entry->length;
            if (entry->length > largest_region_size) {
                largest_region_size = entry->length;
                largest_region_base = entry->base;
            }
        }
    }

    // Step 2: Initialize the bitmap in the largest usable region
    pmm_total_frames = total_memory / PAGE_SIZE;
    bitmap_size = (pmm_total_frames + 7) / 8; // 1 bit per frame, rounded up

    // Place the bitmap at the start of the largest region (using HHDM)
    pmm_bitmap = (uint8_t*)(largest_region_base + hhdm_request.response->offset);

    // Mark all memory as "used" initially
    memset(pmm_bitmap, 0xFF, bitmap_size);

    // Step 3: Iterate through all usable regions and mark them as free
    for (uint64_t i = 0; i < memmap_entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            uint64_t start = entry->base;
            uint64_t end = entry->base + entry->length;

            // Override: Do not mark the first 1MB as free
            if (start < 0x100000) {
                start = 0x100000; // Start from 1MB instead of 0x0
                if (end <= start) continue; // Skip if the entire region is below 1MB
            }

            uint64_t start_frame = start / PAGE_SIZE;
            uint64_t end_frame = (end + PAGE_SIZE - 1) / PAGE_SIZE;

            for (uint64_t j = start_frame; j < end_frame; j++) {
                pmm_bitmap[j / 8] &= ~(1 << (j % 8)); // Mark as free
            }
        }
    }

    // Step 5: Explicitly mark the first 1MB as used
    uint64_t first_mb_start_frame = 0;
    uint64_t first_mb_end_frame = 0x100000 / PAGE_SIZE;
    for (uint64_t j = first_mb_start_frame; j < first_mb_end_frame; j++) {
        pmm_bitmap[j / 8] |= (1 << (j % 8)); // Mark as used
    }


    // Step 4: Mark the bitmap's own memory as "used"
    uint64_t bitmap_start_frame = largest_region_base / PAGE_SIZE;
    uint64_t bitmap_end_frame = (largest_region_base + bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint64_t j = bitmap_start_frame; j < bitmap_end_frame; j++) {
        pmm_bitmap[j / 8] |= (1 << (j % 8)); // Set bit (mark as used)
    }

    // Print some useful information
    kprintf("Total memory: %lu MB\n", total_memory / 1024 / 1024);
    kprintf("Total frames: %lu\n", pmm_total_frames);
    kprintf("Bitmap size: %lu KB\n", bitmap_size / 1024);
    kprintf("Bitmap address: %p\n", pmm_bitmap);
    kprintf("Bitmap start: %lx\n", largest_region_base);
    kprintf("Bitmap end: %lx\n", largest_region_base + bitmap_size);
    kprintf("Bitmap start frame: %lu\n", bitmap_start_frame);
    kprintf("Bitmap end frame: %lu\n", bitmap_end_frame);
    

    pmm_used_frames += (bitmap_end_frame - bitmap_start_frame);
}

void print_and_init_memmap(struct limine_memmap_request memmap_request, struct limine_hhdm_request hhdm_request) {
    struct limine_memmap_response *memmap = memmap_request.response;
    kprintf("Memory Map:\n");
    for (uint64_t i = 0; i < memmap->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap->entries[i];
        kprintf("  Base: 0x%lx, Length: 0x%lx, Type: %d\n",
               entry->base, entry->length, entry->type);
    }

    // Initialize PMM after printing
    pmm_init(memmap_request, hhdm_request);
    kprintf("PMM initialized!\n");
    kprintf("Total frames: %lu, Used frames: %lu\n", pmm_total_frames, pmm_used_frames);
}