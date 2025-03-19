#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include "text_renderer.h"
#include <stdarg.h>
#include "serial.h"
#include "limine_requests.h"

uint64_t* fb_address;
uint64_t fb_width;
uint64_t fb_height;
uint64_t fb_pitch;


// Font size
#define FONT_WIDTH  8
#define FONT_HEIGHT 8

// Cursor position
static size_t cursor_x = 0;
static size_t cursor_y = 0;

// Calculate screen dimensions in characters
static size_t get_screen_width() {
    return fb_width / FONT_WIDTH;
}

static size_t get_screen_height() {
    return fb_height / FONT_HEIGHT;
}

// Font bitmap data would be here (we're skipping as requested)
const uint8_t font[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 (Space)
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // 33 (!)
    {0x36,0x36,0x24,0x00,0x00,0x00,0x00,0x00}, // 34 (")
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // 35 (#)
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // 36 ($)
    {0x63,0x66,0x0C,0x18,0x30,0x66,0x46,0x00}, // 37 (%)
    {0x1C,0x36,0x1C,0x3B,0x6E,0x36,0x1D,0x00}, // 38 (&)
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, // 39 (')
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // 40 (()
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 ())
    {0x00,0x36,0x1C,0x7F,0x1C,0x36,0x00,0x00}, // 42 (*)
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // 43 (+)
    {0x00,0x00,0x00,0x00,0x0C,0x0C,0x18,0x00}, // 44 (,)
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // 45 (-)
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 (.)
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // 47 (/)
    {0x3E,0x63,0x67,0x6F,0x7B,0x73,0x3E,0x00}, // 48 (0)
    {0x0C,0x1C,0x3C,0x0C,0x0C,0x0C,0x3F,0x00}, // 49 (1)
    {0x1E,0x33,0x03,0x06,0x0C,0x18,0x3F,0x00}, // 50 (2)
    {0x1E,0x33,0x03,0x0E,0x03,0x33,0x1E,0x00}, // 51 (3)
    {0x06,0x0E,0x1E,0x36,0x7F,0x06,0x06,0x00}, // 52 (4)
    {0x3F,0x30,0x3E,0x03,0x03,0x33,0x1E,0x00}, // 53 (5)
    {0x1E,0x30,0x3E,0x33,0x33,0x33,0x1E,0x00}, // 54 (6)
    {0x3F,0x03,0x06,0x0C,0x18,0x18,0x18,0x00}, // 55 (7)
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // 56 (8)
    {0x1E,0x33,0x33,0x1F,0x03,0x06,0x3C,0x00}, // 57 (9)
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // 58 (:)
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // 59 (;)
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // 60 (<)
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // 61 (=)
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 (>)
    {0x1E,0x33,0x03,0x06,0x0C,0x00,0x0C,0x00}, // 63 (?)
    {0x3E,0x63,0x6F,0x6B,0x6F,0x60,0x3E,0x00}, // 64 (@)
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, // 65 (A)
    {0x3E,0x33,0x3E,0x33,0x33,0x33,0x3E,0x00}, // 66 (B)
    {0x1E,0x33,0x30,0x30,0x30,0x33,0x1E,0x00}, // 67 (C)
    {0x3C,0x36,0x33,0x33,0x33,0x36,0x3C,0x00}, // 68 (D)
    {0x3F,0x30,0x3E,0x30,0x30,0x30,0x3F,0x00}, // 69 (E)
    {0x3F,0x30,0x3E,0x30,0x30,0x30,0x30,0x00}, // 70 (F)
    {0x1E,0x33,0x30,0x37,0x33,0x33,0x1E,0x00}, // 71 (G)
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, // 72 (H)
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 73 (I)
    {0x07,0x03,0x03,0x03,0x33,0x33,0x1E,0x00}, // 74 (J)
    {0x33,0x36,0x3C,0x38,0x3C,0x36,0x33,0x00}, // 75 (K)
    {0x30,0x30,0x30,0x30,0x30,0x30,0x3F,0x00}, // 76 (L)
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, // 77 (M)
    {0x33,0x3B,0x3F,0x37,0x33,0x33,0x33,0x00}, // 78 (N)
    {0x1E,0x33,0x33,0x33,0x33,0x33,0x1E,0x00}, // 79 (O)
    {0x3E,0x33,0x33,0x3E,0x30,0x30,0x30,0x00}, // 80 (P)
    {0x1E,0x33,0x33,0x33,0x37,0x36,0x1D,0x00}, // 81 (Q)
    {0x3E,0x33,0x33,0x3E,0x3C,0x36,0x33,0x00}, // 82 (R)
    {0x1E,0x33,0x30,0x1E,0x03,0x33,0x1E,0x00}, // 83 (S)
    {0x3F,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00}, // 84 (T)
    {0x33,0x33,0x33,0x33,0x33,0x33,0x1E,0x00}, // 85 (U)
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 86 (V)
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 (W)
    {0x33,0x33,0x1E,0x0C,0x1E,0x33,0x33,0x00}, // 88 (X)
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x0C,0x00}, // 89 (Y)
    {0x3F,0x03,0x06,0x0C,0x18,0x30,0x3F,0x00}, // 90 (Z)
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, // 91 ([)
    {0x00,0x30,0x18,0x0C,0x06,0x03,0x00,0x00}, // 92 (\)
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, // 93 (])
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // 94 (^)
    {0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0x00}, // 95 (_)
    {0x0C,0x0C,0x06,0x00,0x00,0x00,0x00,0x00}, // 96 (`)
    {0x00,0x00,0x1E,0x03,0x1F,0x33,0x1F,0x00}, // 97 (a)
    {0x30,0x30,0x3E,0x33,0x33,0x33,0x3E,0x00}, // 98 (b)
    {0x00,0x00,0x1E,0x33,0x30,0x33,0x1E,0x00}, // 99 (c)
    {0x03,0x03,0x1F,0x33,0x33,0x33,0x1F,0x00}, // 100 (d)
    {0x00,0x00,0x1E,0x33,0x3F,0x30,0x1E,0x00}, // 101 (e)
    {0x0E,0x18,0x3E,0x18,0x18,0x18,0x18,0x00}, // 102 (f)
    {0x00,0x1F,0x33,0x33,0x1F,0x03,0x1E,0x00}, // 103 (g)
    {0x30,0x30,0x3E,0x33,0x33,0x33,0x33,0x00}, // 104 (h)
    {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x1E,0x00}, // 105 (i)
    {0x06,0x00,0x06,0x06,0x06,0x36,0x1C,0x00}, // 106 (j)
    {0x30,0x30,0x33,0x36,0x3C,0x36,0x33,0x00}, // 107 (k)
    {0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 108 (l)
    {0x00,0x00,0x36,0x7F,0x6B,0x63,0x63,0x00}, // 109 (m)
    {0x00,0x00,0x3E,0x33,0x33,0x33,0x33,0x00}, // 110 (n)
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, // 111 (o)
    {0x00,0x00,0x3E,0x33,0x33,0x3E,0x30,0x30}, // 112 (p)
    {0x00,0x00,0x1F,0x33,0x33,0x1F,0x03,0x03}, // 113 (q)
    {0x00,0x00,0x36,0x3B,0x30,0x30,0x30,0x00}, // 114 (r)
    {0x00,0x00,0x1E,0x30,0x1E,0x03,0x3E,0x00}, // 115 (s)
    {0x18,0x18,0x3E,0x18,0x18,0x18,0x0E,0x00}, // 116 (t)
    {0x00,0x00,0x33,0x33,0x33,0x33,0x1F,0x00}, // 117 (u)
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 118 (v)
    {0x00,0x00,0x63,0x6B,0x7F,0x36,0x36,0x00}, // 119 (w)
    {0x00,0x00,0x33,0x1E,0x0C,0x1E,0x33,0x00}, // 120 (x)
    {0x00,0x00,0x33,0x33,0x1F,0x03,0x1E,0x00}, // 121 (y)
    {0x00,0x00,0x3F,0x06,0x0C,0x18,0x3F,0x00}, // 122 (z)
    {0x0E,0x18,0x18,0x30,0x18,0x18,0x0E,0x00}, // 123 ({)
    {0x0C,0x0C,0x0C,0x00,0x0C,0x0C,0x0C,0x00}, // 124 (|)
    {0x38,0x0C,0x0C,0x06,0x0C,0x0C,0x38,0x00}, // 125 (})
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}  // 126 (~)
};


// Draw pixel on the framebuffer
void put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= fb_width || y >= fb_height)
        return; // Bounds checking
        
    uint32_t *fb = (uint32_t*)fb_address;
    fb[y * (fb_pitch / 4) + x] = color;
}

// Render a single character
void draw_char(int x, int y, char c, uint32_t color) {
    if (c < 32 || c > 126) 
        {
            /* Print one simple block*/
            for (int row = 0; row < FONT_HEIGHT; row++) {
                for (int col = 0; col < FONT_WIDTH; col++) {
                    if (row == 0 || row == FONT_HEIGHT - 1 || col == 0 || col == FONT_WIDTH - 1) {
                        put_pixel(x + col, y + row, color);
                    }
                }
            }
            return;
        } // Unsupported characters

    const uint8_t *glyph = font[c - 32];
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (glyph[row] & (1 << (7 - col))) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

// Clear screen by filling it with black
void clear_screen() {
    
    uint32_t *fb = (uint32_t*)fb_address;
    int pixels = fb_height * (fb_pitch / 4);
    
    for (int i = 0; i < pixels; i++) {
        fb[i] = 0x000000; // Black
    }
}

// Scroll screen up by one line
void scroll_screen() {
    
    uint32_t *fb = (uint32_t*)fb_address;
    int pitch = fb_pitch / 4;

    // Copy each row up one line
    for (int y = 0; y < fb_height - FONT_HEIGHT; y++) {
        for (int x = 0; x < fb_width; x++) {
            fb[y * pitch + x] = fb[(y + FONT_HEIGHT) * pitch + x];
        }
    }

    // Clear last row
    for (int y = fb_height - FONT_HEIGHT; y < fb_height; y++) {
        for (int x = 0; x < fb_width; x++) {
            fb[y * pitch + x] = 0x000000;
        }
    }
}

// Print character to screen, handling scrolling and newlines
void putc(char c) {
    
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\r') {
        cursor_x = 0;
    } else if (c == '\t') {
        // Tab advances by 4 spaces
        cursor_x = (cursor_x + 4) & ~3;
    } else if (c >= 32 && c <= 126) {
        draw_char(cursor_x * FONT_WIDTH, cursor_y * FONT_HEIGHT, c, 0xFFFFFF);
        cursor_x++;
    }

    serial_putc(c);

    // Wrap text
    if (cursor_x >= get_screen_width()) {
        cursor_x = 0;
        cursor_y++;
    }

    // Scroll if needed
    if (cursor_y >= get_screen_height()) {
        scroll_screen();
        cursor_y = get_screen_height() - 1;
    }


}

// Print string
void puts(const char *s) {
    if (!s) return;
    
    while (*s) putc(*s++);
}

// Convert integer to string (base 10 or 16)
void itoa(int value, char *str, int base) {
    char *ptr = str, *ptr1 = str, tmp_char;
    int tmp_value;

    if (value < 0 && base == 10) {
        *ptr++ = '-';
        value = -value;
    }

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789ABCDEF"[tmp_value % base];
    } while (value);

    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}


void utoa(uint64_t value, char *str, int base) {
    char *ptr = str, *ptr1 = str, tmp_char;
    uint64_t tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789ABCDEF"[tmp_value % base];
    } while (value);

    *ptr-- = '\0';

    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}


// Kernel printf implementation
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    char buffer[32];

    for (; *fmt; fmt++) {
        if (*fmt == '%') {
            fmt++;

            if (*fmt == 'l') {
                fmt++;
                if (*fmt == 'x') { // Handle %lx
                    utoa(va_arg(args, uint64_t), buffer, 16);
                    puts(buffer);
                } else if (*fmt == 'u') { // Handle %lu
                    utoa(va_arg(args, unsigned long), buffer, 10);
                    puts(buffer);
                } else { // Unsupported 'l' case
                    putc('%');
                    putc('l');
                    putc(*fmt);
                }
            } else {
                switch (*fmt) {
                    case 's':
                        puts(va_arg(args, char*));
                        break;
                    case 'd':
                        itoa(va_arg(args, int), buffer, 10);
                        puts(buffer);
                        break;
                    case 'x':
                        utoa(va_arg(args, unsigned int), buffer, 16);
                        puts(buffer);
                        break;
                    case 'p': { // Handle %p
                        uint64_t ptr = (uint64_t)va_arg(args, void*);
                        puts("0x");
                        utoa(ptr, buffer, 16);
                        puts(buffer);
                        break;
                    }
                    case 'u': // Handle %u (unsigned int)
                        utoa(va_arg(args, unsigned int), buffer, 10);
                        puts(buffer);
                        break;
                    case '%':
                        putc('%');
                        break;
                    default:
                        putc('%');
                        putc(*fmt);
                        break;
                }
            }
        } else {
            putc(*fmt);
        }
    }

    va_end(args);
}




// Initialize framebuffer and clear screen
bool init_text_renderer(uint64_t* addr, uint64_t width, uint64_t height, uint64_t pitch) {

    fb_address = addr;
    fb_width = width;
    fb_height = height;
    fb_pitch = pitch;

    // // Check if framebuffer is available
    // if (framebuffer_request.response == NULL || 
    //     framebuffer_request.response->framebuffer_count < 1) {
    //     return false;
    // }
    
    //framebuffer = framebuffer_request.response->framebuffers[0];
    
    // Validate framebuffer
    // if (!framebuffer || !fb_address) {
    //     return false;
    // }
    
    clear_screen();
    cursor_x = 0;
    cursor_y = 0;

    serial_init();

    kprintf("\n Framebuffer data at %p\n and pointer is of size %d\n", fb_address, sizeof(fb_address));
    return true;
}