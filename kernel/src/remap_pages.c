#include "remap_pages.h"
#include "text_renderer.h"
#include "pmm_mngr.h"
#include "limine_requests.h"
#include "limine.h"
#include "vmm_mngr.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>


#include <stdint.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define RECURSIVE_INDEX 510

/**
 * setup_recursive_mapping - Sets up the recursive mapping in the PML4.
 *
 * @pml4:          Virtual address pointer to the PML4 table.
 * @pml4_phys:     Physical address of the PML4 table.
 *
 * This function writes the physical address of the PML4 table into the
 * 510th entry of the PML4 with the flags to mark it present and writable.
 */
void setup_recursive_mapping(virt_addr_t *pml4, phys_addr_t pml4_phys) {
    // Set the 510th entry to point to the PML4 itself.
    pml4[RECURSIVE_INDEX] = pml4_phys | PAGE_PRESENT | PAGE_WRITE;
}


// Translate a physical address to its virtual address using HHDM.
// (This is only used during early setup.)
uint64_t temp_phys_to_virt(uint64_t phys_addr) {
    return phys_addr + hhdm_request.response->offset;
}

uint64_t temp_virt_to_phys(uint64_t virt_addr) {
    return virt_addr - hhdm_request.response->offset;
}

// The base address of the stack from rsp or rbp?
// Ans: rsp

// Set the stack base address dynamically based on the current stack pointer
// (This is only used during early setup.)

static uint64_t get_limine_stack_top() {
    uint64_t stack_base;
    asm volatile ("mov %%rbp, %0" : "=r"(stack_base));
    return stack_base;
}

static uint64_t get_limine_stack_bottom() {
    uint64_t stack_bottom;
    asm volatile ("mov %%rsp, %0" : "=r"(stack_bottom));
    return stack_bottom;
}


uint64_t new_stack_top;


// Define Global new stack top and bottom pointers

/*
 * remap_kernel() creates a new PML4, then copies the entire 511th kernel mapping
 * (i.e. the kernel's PDP, PD, and PT hierarchies) into freshly allocated pages.
 * It then sets up a recursive mapping (using PML4 entry 510) so that later the CPU
 * can access the page table hierarchy without relying on HHDM.
 */

void print_paging_structure(uint64_t* pml4) {

    for (int pml4_index = 0; pml4_index < 512; pml4_index++) {
        if(pml4_index ==256 ){
            kprintf("Skipping self-referential entry\n");
            continue;
        }
        if (!(pml4[pml4_index] & 1)) continue;  // Skip non-present entries

        uint64_t pdp_phys = pml4[pml4_index] & ~0xFFF;
        uint64_t* pdp = (uint64_t*)temp_phys_to_virt(pdp_phys);

        kprintf("PML4[%d] -> %p\n", pml4_index, pdp_phys);

        for (int pdp_index = 0; pdp_index < 512; pdp_index++) {
            if (!(pdp[pdp_index] & 1)) continue;  

            uint64_t pd_phys = pdp[pdp_index] & ~0xFFF;
            uint64_t* pd = (uint64_t*)temp_phys_to_virt(pd_phys);

            kprintf("  PDP[%d] -> %p\n", pdp_index, pd_phys);

            for (int pd_index = 0; pd_index < 512; pd_index++) {
                if (!(pd[pd_index] & 1)) continue;  

                uint64_t pt_phys = pd[pd_index] & ~0xFFF;

                if (pd[pd_index] & (1ULL << 7)) {  // Huge page (2MB)
                    kprintf("    PD[%d] -> Huge Page %p\n", pd_index, pt_phys);
                    continue;
                }

                uint64_t* pt = (uint64_t*)temp_phys_to_virt(pt_phys);
                kprintf("    PD[%d] -> %p\n", pd_index, pt_phys);

                for (int pt_index = 0; pt_index < 512; pt_index++) {
                    if (!(pt[pt_index] & 1)) continue;  
                    kprintf("      PT[%d] -> %p\n", pt_index, pt[pt_index] & ~0xFFF);
                }
            }
        }
    }
}

void remap_stack(uint64_t *new_pml4) {

    uint64_t new_pdp_phys_stack = pmm_alloc();
    uint64_t* new_pdp_stack = (uint64_t*)temp_phys_to_virt(new_pdp_phys_stack);
    memset(new_pdp_stack, 0, 4096);

    new_pml4[257] = new_pdp_phys_stack | 0x3; // Present + Write

    print_paging_structure(new_pml4);

    // Allocate a new Page Directory (PD) for the stack
    uint64_t new_stack_pd_phys = pmm_alloc();
    uint64_t* new_stack_pd = (uint64_t*)temp_phys_to_virt(new_stack_pd_phys);
    memset(new_stack_pd, 0, 4096);

    // Update the new PDP to point to the new stack PD
    new_pdp_stack[0] = new_stack_pd_phys | 0x3; // Present + Write

    uint64_t new_pt_phys = pmm_alloc();
    uint64_t* new_pt = (uint64_t*)temp_phys_to_virt(new_pt_phys);
    memset(new_pt, 0, 4096);


    // Update the PD entry to point to the new PT
    new_stack_pd[0] = new_pt_phys | 0x3; // Present + Write

    print_paging_structure(new_pml4);

    // Map each 4KB page (1 MiB total)
    // Map each 4KB page (1 MiB total, 256 pages)
    for (int j = 0; j <256; j++) {  // 256 iterations
        uint64_t virt_addr;

        // Use the pre-existing stack page if still within its limits.
        
        uint64_t new_addr = pmm_alloc();
        virt_addr = temp_phys_to_virt(new_addr);
        uint64_t phys_addr = temp_virt_to_phys(virt_addr);
        new_pt[j] = phys_addr | 0x3; // Present + Write
    }

    //print_paging_structure(new_pml4);


    // Copy STACK_SIZE bytes from the old stack to the new stack
    uint64_t old_stack_bottom = get_limine_stack_bottom();
    uint64_t old_stack_top = get_limine_stack_top();

    new_stack_top = ((uint64_t)257 << 39) |
    ((uint64_t)0 << 30) |
    ((uint64_t)0 << 21) |
    ((uint64_t)255 << 12);

    new_stack_top |= 0xFFFF000000000000;
    // return;

    return;
}


void remap_frame_buffer(uint64_t *new_pml4) {
    struct limine_framebuffer* old_framebuffer = framebuffer_request.response->framebuffers[0];

    // Get the physical address of the framebuffer
    uint64_t old_framebuffer_phys = temp_virt_to_phys((uint64_t)old_framebuffer->address);

    // Calculate the size of the framebuffer
    uint64_t framebuffer_size = old_framebuffer->height * old_framebuffer->pitch;

    // Allocate and initialize a PDP entry
    uint64_t framebuffer_pdp_phys = pmm_alloc();
    uint64_t* framebuffer_pdp = (uint64_t*)temp_phys_to_virt(framebuffer_pdp_phys);
    memset(framebuffer_pdp, 0, 4096);

    // Map the PDP entry into the PML4
    new_pml4[258] = framebuffer_pdp_phys | 0x3; // Present + Writable

    // Allocate and initialize a PD entry
    uint64_t framebuffer_pd_phys = pmm_alloc();
    uint64_t* framebuffer_pd = (uint64_t*)temp_phys_to_virt(framebuffer_pd_phys);
    memset(framebuffer_pd, 0, 4096);

    // Map the PD entry into the PDP
    framebuffer_pdp[0] = framebuffer_pd_phys | 0x3; // Present + Writable

    // Calculate the number of PTs required
    uint64_t num_pt = (framebuffer_size + 2 * 1024 * 1024 - 1) / (2 * 1024 * 1024);

    // Allocate and map PTs
    for (uint64_t i = 0; i < num_pt; i++) {
        // Allocate a PT
        uint64_t framebuffer_pt_phys = pmm_alloc();
        uint64_t* framebuffer_pt = (uint64_t*)temp_phys_to_virt(framebuffer_pt_phys);
        memset(framebuffer_pt, 0, 4096);

        // Map the PT into the PD
        framebuffer_pd[i] = framebuffer_pt_phys | 0x3; // Present + Writable

        // Map the framebuffer memory into the PT
        for (uint64_t j = 0; j < 512; j++) {
            uint64_t phys_addr = old_framebuffer_phys + (i * 2 * 1024 * 1024) + (j * 4096);
            framebuffer_pt[j] = phys_addr | 0x3; // Present + Writable
        }
    }

    uint64_t new_framebuffer_virt_addr;


    new_framebuffer_virt_addr = ((uint64_t)258 << 39) |
    ((uint64_t)0 << 30) |
    ((uint64_t)0 << 21) |
    ((uint64_t)0 << 12);

    new_framebuffer_virt_addr |= 0xFFFF000000000000;

    framebuffer_request.response->framebuffers[0]->address = (uint64_t* )new_framebuffer_virt_addr;

    //init_text_renderer(new_framebuffer_virt_addr, old_framebuffer->width, old_framebuffer->height, old_framebuffer->pitch);


    // Update the framebuffer structure



}



void remap_kernel() {
    uint64_t cr3 = read_cr3();
    uint64_t* old_pml4 = (uint64_t*)temp_phys_to_virt(cr3);

    // Remap the stack
    remap_stack(old_pml4);

    //remap_frame_buffer(old_pml4);

    // PML4[510] for recursive mapping. No, write explicitly. Do not allocate anymore space
    setup_recursive_mapping(old_pml4, cr3);

    kprintf("Stack Working!!");

    return;

}