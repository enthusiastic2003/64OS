#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "text_renderer.h"
#include "pmm_mngr.h"
#include "string.h"
#include "pagining_mgr.h"
#include "limine_requests.h"
#include "remap_pages.h"
#include "idt.h"

extern uint64_t _end;
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
void kmain(void) {
    // Ensure the bootloader actually understands our base revision (see spec).
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        hcf();
    }

    bool fb_init = init_text_renderer(framebuffer_request.response->framebuffers[0]->address, framebuffer_request.response->framebuffers[0]->width, framebuffer_request.response->framebuffers[0]->height, framebuffer_request.response->framebuffers[0]->pitch);

    if (!fb_init) {
        hcf();
    }

    kprintf("RSP: %p\n", get_limine_stack_base());
    kprintf("RBP: %p\n", get_limine_stack_bottom());

    kprintf("Kernel loaded at physical: %p\n", &kmain);

    kprintf("cr3: %lx\n",hhdm_request.response->offset + read_cr3());
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
    kprintf("-------------------------\n");
    kprintf("Existing Pages Test\n");
    kprintf("-------------------------\n");

    inspect_page_tables(hhdm_request);

    kprintf("-------------------------\n");
    kprintf("Other Tests tests\n");
    kprintf("-------------------------\n");
    /*
    void *address;
    uint64_t size;
    char *path;
    char *string;

    Print them all
    */
    kprintf("Address: %p\n", exec_file.response->kernel_file->address);
    kprintf("Size: %lu\n", exec_file.response->kernel_file->size);
    kprintf("Path: %s\n", exec_file.response->kernel_file->path);
    kprintf("Media type: %u\n", exec_file.response->kernel_file->media_type);
    kprintf("-------------------------\n");
    kprintf("Limine Response Data Structures Pointers\n");
    kprintf("-------------------------\n");

    kprintf("limine_memmap_request response: %p\n, end: %p\n", memmap_request.response, sizeof(memmap_request.response) + memmap_request.response);
    kprintf("limine_hhdm_request response: %p\n, end: %p\n", hhdm_request.response, sizeof(hhdm_request.response) + hhdm_request.response);
    kprintf("limine_executable_file_request response: %p\n, end: %p\n", exec_file.response, sizeof(exec_file.response) + exec_file.response);

//    preserve_limine_requests();

//   init_text_renderer();
//    init_text_renderer();
    kprintf("Preserved Limine requests!!\n");
    // Print the reloacted pointers
    kprintf("limine_hhdm_request response: %p\n, end: %p\n", hhdm_request.response, sizeof(hhdm_request.response) + hhdm_request.response);
    kprintf("limine_executable_file_request response: %p\n, end: %p\n", exec_file.response, sizeof(exec_file.response) + exec_file.response);
    kprintf("limine_framebuffer_request response: %p\n, end: %p\n", framebuffer_request.response, sizeof(framebuffer_request.response) + framebuffer_request.response);
    kprintf("limine_memmap_request response: %p\n, end: %p\n", memmap_request.response, sizeof(memmap_request.response) + memmap_request.response);
    
    kprintf("Kernel end: %p\n", &_end);
    //inspect_page_tables();

    remap_kernel();


    ///////////////////////////////////////////////////////////////
    // !!!!!!!!!!!HHDM IS NOW DISABLED!!!!!!!!!!!
    // !!!!!!CANNOT REFERENCE HHDM REQUESTS!!!!!!
    // !!!!NEED TO IMPLEMENT NEW PROCEDURE!!!!!!!
    ///////////////////////////////////////////////////////////////

    uint64_t old_stack_top = get_limine_stack_bottom();
    uint64_t old_stack_bottom = get_limine_stack_base();

    asm volatile("mov %0, %%rbp" :: "r"(new_stack_top));
    asm volatile("mov %0, %%rsp" :: "r"(new_stack_top- (old_stack_top - old_stack_bottom)));


    kprintf("New RSP: %p\n", get_limine_stack_base());
    kprintf("New RBP: %p\n", get_limine_stack_bottom());


    kprintf("If you are seeing this, the stack has been remapped successfully\n");
    kprintf("Also disabled hhdm\n");

    idt_install();
    kprintf("IDT installed\n");

    // Test huge pages

    // Try to page fault:

    uint64_t *ptr = (uint64_t *)0x1000;
    *ptr = 0x12345678;

    //test_huge_pages();


    
    
    hcf();




}
