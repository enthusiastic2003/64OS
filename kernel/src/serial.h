#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>

// Write a byte to an I/O port
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Read a byte from an I/O port
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#define COM1 0x3F8  // COM1 serial port base address

// Initialize the serial port
void serial_init() {
    outb(COM1 + 1, 0x00);  // Disable interrupts
    outb(COM1 + 3, 0x80);  // Enable DLAB (Divisor Latch Access)
    outb(COM1 + 0, 0x03);  // Set divisor to 3 (38400 baud)
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);  // 8 bits, no parity, one stop bit
    outb(COM1 + 2, 0xC7);  // Enable FIFO, clear them, 14-byte threshold
    outb(COM1 + 4, 0x0B);  // Enable IRQs, RTS/DSR set
}

// Send a character to the serial port
void serial_putc(char c) {
    while ((inb(COM1 + 5) & 0x20) == 0);  // Wait until the buffer is empty
    outb(COM1, c);
}

#endif // SERIAL_H

