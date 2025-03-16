#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "text_renderer.h"
#include "pmm_mngr.h"
#include "vmm_mngr.h"
#include "string.h"

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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,    
};


// GCC and Clang reserve the right to generate calls to the following
// 4 functions even if they are not directly called.
// Implement them as the C specification mandates.
// DO NOT remove or rename these functions, or stuff will eventually break!
// They CAN be moved to a different .c file.

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
    uint64_t rsp = read_cr3();
    kprintf("Current stack pointer: %p\n", rsp);
    kprintf("cr3: %lx\n", hhdm_request.response->offset + read_cr3());
    print_and_init_memmap(memmap_request, hhdm_request);


    kprintf("-------------------------\n");
    kprintf("PMM tests\n");
    kprintf("-------------------------\n");
    kprintf("Total frames: %lu\n", get_total_frame_count());
    kprintf("Free frames: %lu\n", get_free_frame_count());
    kprintf("Used frames: %lu\n", get_used_frame_count());
    uint64_t frame1 = pmm_alloc();
    uint64_t frame2 = pmm_alloc();
    kprintf("Allocated frames: 0x%lx, 0x%lx\n", frame1, frame2);

    kprintf("Free frames: %lu\n", get_free_frame_count());
    kprintf("Used frames: %lu\n", get_used_frame_count());

    pmm_free(frame1);
    kprintf("Freed 1 frame\n");
    kprintf("Free frames: %lu\n", get_free_frame_count());
    kprintf("Used frames: %lu\n", get_used_frame_count());


    hcf();
}
