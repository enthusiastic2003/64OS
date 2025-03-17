#include "limine_requests.h"

// Define Limine request markers
__attribute__((used, section(".limine_requests_start")))
volatile uint8_t limine_requests_start_marker;

__attribute__((used, section(".limine_requests")))
volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,    
};

__attribute__((used, section(".limine_requests")))
volatile struct limine_kernel_file_request exec_file = {
    .id = LIMINE_KERNEL_FILE_REQUEST,
    .revision = 0,
};

// Framebuffer request
__attribute__((used, section(".limine_requests")))
volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// Define Limine request end marker
__attribute__((used, section(".limine_requests_end")))
volatile uint8_t limine_requests_end_marker;
