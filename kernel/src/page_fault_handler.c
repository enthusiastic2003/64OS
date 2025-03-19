#include <stdint.h>
#include "page_fault_handler.h"
#include "text_renderer.h"

/**
 * page_fault_handler - Handles page fault exceptions.
 *
 * @frame: Pointer to the CPU state at the time of the fault.
 * @error_code: The error code provided by the CPU.
 *
 * This handler prints the faulting virtual address (from CR2) and halts the system.
 */
__attribute__((interrupt))
void page_fault_handler(struct interrupt_frame *frame, uint64_t error_code) {
    uint64_t fault_addr;
    // Retrieve the faulting address from CR2.
    asm volatile ("mov %%cr2, %0" : "=r" (fault_addr));

    // Print out the faulting virtual address.
    kprintf("Page fault at virtual address: 0x%lx\n", fault_addr);
    kprintf("Error code: 0x%lx\n", error_code);

    // Halt the system.
    while (1) {
        asm volatile ("hlt");
    }
}
