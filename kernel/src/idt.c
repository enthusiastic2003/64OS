#include "idt.h"
#include "page_fault_handler.h"
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


void idt_install(void) {
    // Set up the IDT pointer.
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base  = (uint64_t)&idt;

    // Clear the IDT.
    memset(idt, 0, sizeof(idt_entry_t) * IDT_ENTRIES);

    // -- Setup your IDT entries here --
    // For example, if you have a page fault handler:
    // extern void page_fault_handler(struct interrupt_frame*, uint64_t);
    // idt_set_gate(14, (uint64_t)page_fault_handler, 0x08, 0x8E);

    // Set the interrupt gate for the page fault handler
    idt_set_gate(14, (uint64_t)page_fault_handler, 0x28, 0x8E);


    // You can set other handlers (e.g., for general protection faults, timer interrupts, etc.)

    // Load the IDT using the lidt instruction.
    asm volatile("lidt %0" : : "m"(idt_ptr));
}
