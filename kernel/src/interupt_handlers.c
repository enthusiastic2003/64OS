#include <stdint.h>

// Simple console output function (you'll need to implement this based on your kernel)
#include "text_renderer.h"

// Interrupt frame structure
struct interrupt_frame {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

// Handlers without error codes
void divide_by_zero_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Divide by Zero Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to divide by zero");
}

void debug_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Debug Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to debug exception");
}

void nmi_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Non-Maskable Interrupt at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to NMI");
}

void breakpoint_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Breakpoint Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to breakpoint");
}

void overflow_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Overflow Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to overflow");
}

void bound_range_exceeded_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Bound Range Exceeded at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to bound range exceeded");
}

void invalid_opcode_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Invalid Opcode at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to invalid opcode");
}

void device_not_available_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Device Not Available at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to device not available");
}

void x87_floating_point_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: x87 Floating-Point Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to x87 floating-point exception");
}

void machine_check_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Machine Check Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to machine check exception");
}

void simd_floating_point_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: SIMD Floating-Point Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to SIMD floating-point exception");
}

void virtualization_handler(struct interrupt_frame* frame) {
    kprintf("FAULT: Virtualization Exception at RIP: 0x%lx\n", frame->rip);
    kprintf("System halted due to virtualization exception");
}

// Handlers with error codes
void double_fault_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: Double Fault at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to double fault");
}

void invalid_tss_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: Invalid TSS at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to invalid TSS");
}

void segment_not_present_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: Segment Not Present at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to segment not present");
}

void stack_segment_fault_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: Stack-Segment Fault at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to stack-segment fault");
}

void general_protection_fault_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: General Protection Fault at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to general protection fault");
}

void alignment_check_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: Alignment Check at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to alignment check");
}

void control_protection_handler(struct interrupt_frame* frame, uint64_t error_code) {
    kprintf("FAULT: Control Protection Exception at RIP: 0x%lx, Error Code: 0x%lx\n", frame->rip, error_code);
    kprintf("System halted due to control protection exception");
}

// Page fault handler (if you don't have one already)
void page_fault_handler(struct interrupt_frame* frame, uint64_t error_code) {
    uint64_t fault_addr;
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr));
    
    kprintf("FAULT: Page Fault at RIP: 0x%lx, Fault Address: 0x%lx, Error Code: 0x%lx\n", 
            frame->rip, fault_addr, error_code);
    
    // Decode error code
    kprintf("Page Fault Details: %s %s %s %s\n",
            (error_code & 1) ? "Protection Violation" : "Page Not Present",
            (error_code & 2) ? "Write" : "Read",
            (error_code & 4) ? "User Mode" : "Kernel Mode",
            (error_code & 8) ? "Reserved Bit Set" : "");
    
    kprintf("System halted due to page fault");
}