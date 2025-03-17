#pragma once
#define LIMINE_API_REVISION 0

#include <limine.h> // Include the Limine header

// Declare the Limine requests as external variables
extern volatile struct limine_memmap_request memmap_request;
extern volatile struct limine_hhdm_request hhdm_request;
extern volatile struct limine_kernel_file_request exec_file;
extern volatile struct limine_framebuffer_request framebuffer_request;

// Start and end markers for Limine requests
extern volatile uint8_t limine_requests_start_marker;
extern volatile uint8_t limine_requests_end_marker;
