#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "text_renderer.h"
#include "pmm_mngr.h"
#include "string.h"
#include "pagining_mgr.h"
#include "limine_requests.h"
#include "idt.h"
#include "vmm_mngr.h"
#include "printing_utils.h"

extern uint64_t _end;
extern uint64_t *pmm_bitmap;
// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile LIMINE_BASE_REVISION(3);

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

extern uint64_t new_stack_top;
extern uint64_t new_stack_bottom;

// Kernel start and end from linker script
void test_huge_pages() {
    uint64_t kernel_end = (uint64_t)&_end;
    uint64_t test_addr = kernel_end + (512 * 1024); // Kernel end + 512 KiB

    *((volatile uint64_t *)test_addr) = 0xDEADBEEF; // Attempt a write

    // If we reach here, no page fault occurred
    kprintf("Write successful! Huge pages may be used.\n");
}



static inline uint64_t get_limine_stack_base() {
    uint64_t stack_base;
    asm volatile ("mov %%rsp, %0" : "=r"(stack_base));
    return stack_base;
}

static inline uint64_t get_limine_stack_bottom() {
    uint64_t stack_bottom;
    asm volatile ("mov %%rbp, %0" : "=r"(stack_bottom));
    return stack_bottom;
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.

#include <string.h> // for memset and memcpy



void kmain(void) {

    // Disable interrupts
    __asm__ __volatile__("cli");
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }


    idt_install();

    bool fb_init = init_text_renderer(framebuffer_request.response->framebuffers[0]->address, framebuffer_request.response->framebuffers[0]->width, framebuffer_request.response->framebuffers[0]->height, framebuffer_request.response->framebuffers[0]->pitch);

    if (!fb_init) {
        hcf();
    }

    print_and_init_memmap(memmap_request, hhdm_request);

    inspect_page_tables();

    remap_kernel_pages();
    pmm_alloc();
    kprintf("\nNew PMM Initied!!\n");
    hcf();
    vm_buddy_allocator_init();

    virt_addr_t* mypage = (virt_addr_t*) vm_alloc_pages(1);
    kprintf("Received a new virtual page at: %p", mypage);


    hcf();

}
