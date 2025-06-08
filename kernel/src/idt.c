#include "idt.h"
#include <string.h>

// Define the global IDT array and IDT pointer.
idt_entry_t idt[IDT_ENTRIES];
idt_ptr_t idt_ptr;

void idt_set_gate(uint8_t vector, uint64_t handler, uint16_t selector, uint8_t type_attr) {
    idt[vector].offset_low  = handler & 0xFFFF;
    idt[vector].selector    = selector;
    idt[vector].ist         = 0;  // Set to 0 if not using an Interrupt Stack Table.
    idt[vector].type_attr   = type_attr;
    idt[vector].offset_mid  = (handler >> 16) & 0xFFFF;
    idt[vector].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

// External fault handler declarations
extern void divide_by_zero_handler(void);
extern void debug_handler(void);
extern void nmi_handler(void);
extern void breakpoint_handler(void);
extern void overflow_handler(void);
extern void bound_range_exceeded_handler(void);
extern void invalid_opcode_handler(void);
extern void device_not_available_handler(void);
extern void double_fault_handler(void);
extern void invalid_tss_handler(void);
extern void segment_not_present_handler(void);
extern void stack_segment_fault_handler(void);
extern void general_protection_fault_handler(void);
extern void page_fault_handler(void);
extern void x87_floating_point_handler(void);
extern void alignment_check_handler(void);
extern void machine_check_handler(void);
extern void simd_floating_point_handler(void);
extern void virtualization_handler(void);
extern void control_protection_handler(void);

void idt_install(void) {
    // Set up the IDT pointer.
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint64_t)&idt;

    // Clear the IDT.
    memset(idt, 0, sizeof(idt_entry_t) * IDT_ENTRIES);

    // Register CPU exception handlers (vectors 0-21)
    // Note: Using 0x08 as code segment selector for kernel code
    // Type 0x8E = Present, Ring 0, 32-bit Interrupt Gate

    idt_set_gate(0,  (uint64_t)divide_by_zero_handler,       0x08, 0x8E);  // Divide by Zero
    idt_set_gate(1,  (uint64_t)debug_handler,                0x08, 0x8E);  // Debug
    idt_set_gate(2,  (uint64_t)nmi_handler,                  0x08, 0x8E);  // Non-Maskable Interrupt
    idt_set_gate(3,  (uint64_t)breakpoint_handler,           0x08, 0x8E);  // Breakpoint
    idt_set_gate(4,  (uint64_t)overflow_handler,             0x08, 0x8E);  // Overflow
    idt_set_gate(5,  (uint64_t)bound_range_exceeded_handler, 0x08, 0x8E);  // Bound Range Exceeded
    idt_set_gate(6,  (uint64_t)invalid_opcode_handler,       0x08, 0x8E);  // Invalid Opcode
    idt_set_gate(7,  (uint64_t)device_not_available_handler, 0x08, 0x8E);  // Device Not Available
    idt_set_gate(8,  (uint64_t)double_fault_handler,         0x08, 0x8E);  // Double Fault
    // Vector 9 is reserved (Coprocessor Segment Overrun - legacy)
    idt_set_gate(10, (uint64_t)invalid_tss_handler,          0x08, 0x8E);  // Invalid TSS
    idt_set_gate(11, (uint64_t)segment_not_present_handler,  0x08, 0x8E);  // Segment Not Present
    idt_set_gate(12, (uint64_t)stack_segment_fault_handler,  0x08, 0x8E);  // Stack-Segment Fault
    idt_set_gate(13, (uint64_t)general_protection_fault_handler, 0x08, 0x8E);  // General Protection Fault
    idt_set_gate(14, (uint64_t)page_fault_handler,           0x08, 0x8E);  // Page Fault
    // Vector 15 is reserved
    idt_set_gate(16, (uint64_t)x87_floating_point_handler,   0x08, 0x8E);  // x87 Floating-Point Exception
    idt_set_gate(17, (uint64_t)alignment_check_handler,      0x08, 0x8E);  // Alignment Check
    idt_set_gate(18, (uint64_t)machine_check_handler,        0x08, 0x8E);  // Machine Check
    idt_set_gate(19, (uint64_t)simd_floating_point_handler,  0x08, 0x8E);  // SIMD Floating-Point Exception
    idt_set_gate(20, (uint64_t)virtualization_handler,       0x08, 0x8E);  // Virtualization Exception
    idt_set_gate(21, (uint64_t)control_protection_handler,   0x08, 0x8E);  // Control Protection Exception
    // Vectors 22-31 are reserved for future Intel use

    // Load the IDT using the lidt instruction.
    asm volatile("lidt %0" : : "m"(idt_ptr));
}