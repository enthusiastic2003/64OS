#include <stdint.h>

// Structure passed to interrupt handlers (simplified)
struct interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t flags;
    uint64_t rsp;
    uint64_t ss;
};

/**
 * page_fault_handler - Handles page fault exceptions.
 *
 * @frame: Pointer to the CPU state at the time of the fault.
 * @error_code: The error code provided by the CPU.
 *
 * This handler prints the faulting virtual address (from CR2) and halts the system.
 */
__attribute__((interrupt))
void page_fault_handler(struct interrupt_frame *frame, uint64_t error_code) ;