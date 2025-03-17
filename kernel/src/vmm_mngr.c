#include "vmm_mngr.h"
#include "limine_requests.h"
#include "pmm_mngr.h"
#include "string.h"

#define HHDM_OFFSET (hhdm_request.response->offset)
#define VIRT_TO_PHYS(v) ((uint64_t)(v) - HHDM_OFFSET)
#define PHYS_TO_VIRT(p) ((uint64_t)(p) + HHDM_OFFSET)

void preserve_limine_requests() {
    // Allocate memory for each Limine request structure
    uint64_t hhdm_phys = pmm_alloc();
    uint64_t exec_phys = pmm_alloc();
    uint64_t fb_phys = pmm_alloc();

    uint64_t hddm_response_phys = pmm_alloc();
    uint64_t exec_response_phys = pmm_alloc();
    uint64_t fb_response_phys = pmm_alloc();
    uint64_t framebuffer_struct_phys = pmm_alloc();


    if (!hhdm_phys || !exec_phys || !fb_phys || !hddm_response_phys || !exec_response_phys) {
        // Handle memory allocation failure (e.g., panic, log error)
        return;
    }

    // Create response structures
    struct limine_hhdm_response *hhdm_response = (void *)PHYS_TO_VIRT(hddm_response_phys);
    struct limine_kernel_file_response *exec_response = (void *)PHYS_TO_VIRT(exec_response_phys);
    struct limine_framebuffer_response *fb_response = (void *)PHYS_TO_VIRT(fb_response_phys);
    struct limine_framebuffer *framebuffer_struct = (void *)PHYS_TO_VIRT(framebuffer_struct_phys);

    // Compute virtual addresses using HHDM
    volatile struct limine_hhdm_request *new_hhdm_request = (void *)(hhdm_phys + HHDM_OFFSET);
    volatile struct limine_kernel_file_request *new_exec_file = (void *)(exec_phys + HHDM_OFFSET);
    volatile struct limine_framebuffer_request *new_framebuffer_request = (void *)(fb_phys + HHDM_OFFSET);


    // Copy original structures to allocated memory
    memcpy((void *)new_hhdm_request, (void *)&hhdm_request, sizeof(hhdm_request));
    memcpy((void *)new_exec_file, (void *)&exec_file, sizeof(exec_file));
    memcpy((void *)new_framebuffer_request, (void *)&framebuffer_request, sizeof(framebuffer_request));
    
    memcpy((void*)hhdm_response, (void*)hhdm_request.response, sizeof(struct limine_hhdm_response));
    memcpy((void*)exec_response, (void*)exec_file.response, sizeof(struct limine_kernel_file_response));
    memcpy((void*)fb_response, (void*)framebuffer_request.response, sizeof(struct limine_framebuffer_response));

    memcpy((void*)framebuffer_struct, (void*)framebuffer_request.response->framebuffers[0], sizeof(struct limine_framebuffer));
    
    // Update pointers to response structures
    new_hhdm_request->response = hhdm_response;
    new_exec_file->response = exec_response;
    new_framebuffer_request->response = fb_response;

    new_framebuffer_request->response->framebuffers[0] = framebuffer_struct;

    // Update pointers to new structures
    hhdm_request = *new_hhdm_request;
    exec_file = *new_exec_file;
    framebuffer_request = *new_framebuffer_request;


    
}

