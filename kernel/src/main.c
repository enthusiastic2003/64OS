#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "text_renderer.h"
#include "pmm_mngr.h"
#include "string.h"
#include "pagining_mgr.h"
#include "limine_requests.h"
#include "vmm_mngr.h"

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



// Kernel start and end from linker script
void test_huge_pages() {
    uint64_t kernel_end = (uint64_t)&_end;
    uint64_t test_addr = kernel_end + (512 * 1024); // Kernel end + 512 KiB

    *((volatile uint64_t *)test_addr) = 0xDEADBEEF; // Attempt a write

    // If we reach here, no page fault occurred
    kprintf("Write successful! Huge pages may be used.\n");
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

    

    kprintf("Kernel loaded at physical: %p\n", &kmain);
    uint64_t rsp = read_cr3();
    kprintf("Current stack pointer: %p\n", rsp);
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

    preserve_limine_requests();

    init_text_renderer();
    kprintf("Preserved Limine requests!!\n");
    // Print the reloacted pointers
    kprintf("limine_hhdm_request response: %p\n, end: %p\n", hhdm_request.response, sizeof(hhdm_request.response) + hhdm_request.response);
    kprintf("limine_executable_file_request response: %p\n, end: %p\n", exec_file.response, sizeof(exec_file.response) + exec_file.response);
    kprintf("limine_framebuffer_request response: %p\n, end: %p\n", framebuffer_request.response, sizeof(framebuffer_request.response) + framebuffer_request.response);
    kprintf("limine_memmap_request response: %p\n, end: %p\n", memmap_request.response, sizeof(memmap_request.response) + memmap_request.response);
    
    kprintf("Kernel end: %p\n", &_end);
        
    hcf();




}
