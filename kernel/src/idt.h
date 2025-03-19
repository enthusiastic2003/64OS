#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// IDT entry structure for x86_64 (16 bytes per entry)
typedef struct __attribute__((packed)) {
    uint16_t offset_low;   // Lower 16 bits of handler address.
    uint16_t selector;     // Code segment selector in GDT.
    uint8_t  ist;          // Interrupt Stack Table offset (3 bits used) and zeros.
    uint8_t  type_attr;    // Type and attributes (e.g., present, DPL, gate type).
    uint16_t offset_mid;   // Middle 16 bits of handler address.
    uint32_t offset_high;  // Upper 32 bits of handler address.
    uint32_t zero;         // Reserved, set to zero.
} idt_entry_t;

// IDT pointer structure (used by lidt instruction)
typedef struct __attribute__((packed)) {
    uint16_t limit;
    uint64_t base;
} idt_ptr_t;

#define IDT_ENTRIES 256

// Global IDT array declaration
extern idt_entry_t idt[IDT_ENTRIES];

/**
 * idt_set_gate - Set an individual entry in the IDT.
 *
 * @vector:     Interrupt vector number (0-255).
 * @handler:    The address of the interrupt handler function.
 * @selector:   Code segment selector in the GDT (typically 0x08 for kernel code).
 * @type_attr:  Attributes for the gate (for example, 0x8E: present, ring 0, interrupt gate).
 */
void idt_set_gate(uint8_t vector, uint64_t handler, uint16_t selector, uint8_t type_attr);

/**
 * idt_install - Install and load the IDT.
 *
 * This function initializes the IDT pointer, clears the IDT,
 * sets up the required entries (you can add your own after this),
 * and loads the IDT using the lidt instruction.
 */
void idt_install(void);

#ifdef __cplusplus
}
#endif

#endif // IDT_H
