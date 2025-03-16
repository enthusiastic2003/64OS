#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "text_renderer.h"
#include "pagining_mgr.h"
// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

// __attribute__((used, section(".limine_requests")))
// static volatile struct limine_framebuffer_request framebuffer_request = {
//     .id = LIMINE_FRAMEBUFFER_REQUEST,
//     .revision = 0
// };

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile LIMINE_REQUESTS_END_MARKER;


// Request the memory map from Limine
__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// Function to print memory map
void print_memmap(void) {
    if (memmap_request.response == NULL) {
        kprintf("Memory map request failed!\n");
        return;
    }

    kprintf("Memory Map:\n");

    struct limine_memmap_entry **entries = memmap_request.response->entries;
    uint64_t entry_count = memmap_request.response->entry_count;

    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = entries[i];

        kprintf("Region %d: Base: %lx, Length: %lx, Type: %d\n",
                i, entry->base, entry->length, entry->type);
    }
}


// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// Halt and catch fire function.
static void hcf(void) {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}

void dump_pml4() {

    uint64_t cr3 = read_cr3();

    uint64_t *pml4 = (uint64_t *) (0xFFFF800000000000 + cr3);
    
    for (int i = 0; i < 512; i++) {
        if (pml4[i] & 1) {  // Check if present
            kprintf("PML4[%d]: %lx (maps %lx000000000)\n", i, pml4[i], (uint64_t)i);
        }
    }
}


// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    bool fb_init = init_text_renderer();

    if (!fb_init) {
        hcf();
    }

    kprintf("kmain is at: %p\n", (uint64_t)&kmain);
    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=r"(rsp));
    kprintf("Current stack pointer: %p\n", rsp);
    // We're done, just hang...3
    print_memmap();
    kprintf("Printing page mappings...\n");
    kprintf("cr3: %lx\n", 0xFFFF800000000000 + read_cr3());
    dump_pml4();

    hcf();
}
