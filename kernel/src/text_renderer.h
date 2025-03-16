#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the text renderer (sets up framebuffer and clears screen)
bool init_text_renderer();

// Print a single character to the screen
void putc(char c);

// Print a string to the screen
void puts(const char *s);

// Clear the entire screen
void clear_screen();

// Scroll the screen up by one line
void scroll_screen();

void kprintf(const char *fmt, ...);

#endif // TEXT_RENDERER_H
