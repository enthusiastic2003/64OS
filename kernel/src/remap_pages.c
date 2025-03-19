#include "remap_pages.h"
#include "text_renderer.h"
#include "pmm_mngr.h"
#include "limine_requests.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
uint64_t new_stack_bottom;


#define LIMINE_STACK_SIZE 0x10000  // 64 KiB



// Define Global new stack top and bottom pointers

/*
 * remap_kernel() creates a new PML4, then copies the entire 511th kernel mapping
 * (i.e. the kernel's PDP, PD, and PT hierarchies) into freshly allocated pages.
 * It then sets up a recursive mapping (using PML4 entry 510) so that later the CPU
 * can access the page table hierarchy without relying on HHDM.
 */



void t_hcf() {
    for (;;) {
        asm ("hlt");
    }
}





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

__attribute__((noinline)) void vd_breakpoint(){
    return;
}

__attribute__((noinline)) void change_cr3(uint64_t new_pml4_phys) {

    //print old stack pointers


    //Before switching, set the new stack pointers
    asm volatile("mov %0, %%rbp" :: "r"(new_stack_top));

    
    //kprintf("Return address: %p\n", ret_addr);


    //Create a new debug symbol
    asm volatile("mov %0, %%cr3" :: "r"(new_pml4_phys));

    // Get the return address



    return;
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

    print_paging_structure(new_pml4);


    // Copy STACK_SIZE bytes from the old stack to the new stack
    uint64_t old_stack_bottom = get_limine_stack_bottom();
    uint64_t old_stack_top = get_limine_stack_top();

    new_stack_top = ((uint64_t)257 << 39) |
    ((uint64_t)0 << 30) |
    ((uint64_t)0 << 21) |
    ((uint64_t)255 << 12);

    new_stack_top |= 0xFFFF000000000000;

    new_stack_bottom = new_stack_top - (old_stack_top - old_stack_bottom);

    kprintf("old stack size = %p\n", old_stack_top - old_stack_bottom);
    // Copy the old stack to the new stack



    //memcpy((void*) (new_stack_top - LIMINE_STACK_SIZE), (void*) (old_stack_top - LIMINE_STACK_SIZE), LIMINE_STACK_SIZE);

    // Change the stack bottom and top pointers

    //asm volatile("1: jmp 1b");

    // asm volatile("mov %0, %%rbp" :: "r"(new_stack_top));
    // asm volatile("mov %0, %%rsp" :: "r"(new_stack_bottom));
    // return;

    return;
}


void remap_kernel() {
    uint64_t cr3 = read_cr3();
    uint64_t* old_pml4 = (uint64_t*)temp_phys_to_virt(cr3);


    // print_paging_structure(old_pml4);

    // uint64_t new_pml4_phys = pmm_alloc();
    // uint64_t* new_pml4 = (uint64_t*)temp_phys_to_virt(new_pml4_phys);
    // memset(new_pml4, 0, 4096);

    // uint32_t kernel_pml4_entry = 511;
    // if (!(old_pml4[kernel_pml4_entry] & 1))
    //     kprintf("Kernel mapping does not exist!");

    // uint64_t old_pdp_phys = old_pml4[kernel_pml4_entry] & ~0xFFF;
    // uint64_t* old_pdp = (uint64_t*)temp_phys_to_virt(old_pdp_phys);

    // uint64_t new_pdp_phys = pmm_alloc();
    // uint64_t* new_pdp = (uint64_t*)temp_phys_to_virt(new_pdp_phys);
    // memset(new_pdp, 0, 4096);

    // new_pml4[kernel_pml4_entry] = new_pdp_phys | 0x3;

    // int last_used_pdp_index = -1;
    // for (int pdp_index = 0; pdp_index < 512; pdp_index++) {
    //     if (!(old_pdp[pdp_index] & 1))
    //         continue;
        

    //     uint64_t old_pd_phys = old_pdp[pdp_index] & ~0xFFF;
    //     uint64_t* old_pd = (uint64_t*)temp_phys_to_virt(old_pd_phys);

    //     uint64_t new_pd_phys = pmm_alloc();
    //     uint64_t* new_pd = (uint64_t*)temp_phys_to_virt(new_pd_phys);
    //     memset(new_pd, 0, 4096);

    //     new_pdp[pdp_index] = new_pd_phys | 0x3;

    //     for (int pd_index = 0; pd_index < 512; pd_index++) {

    //         if (!(old_pd[pd_index] & 1))
    //             continue;

    //         uint64_t old_pt_phys = old_pd[pd_index] & ~0xFFF;
    //         if (old_pd[pd_index] & (1ULL << 7)) {
    //             new_pd[pd_index] = old_pd[pd_index];
    //             continue;
    //         }

    //         uint64_t* old_pt = (uint64_t*)temp_phys_to_virt(old_pt_phys);
    //         uint64_t new_pt_phys = pmm_alloc();
    //         uint64_t* new_pt = (uint64_t*)temp_phys_to_virt(new_pt_phys);
    //         memset(new_pt, 0, 4096);

    //         new_pd[pd_index] = new_pt_phys | 0x3;

    //         for (int pt_index = 0; pt_index < 512; pt_index++) {
    //             if (!(old_pt[pt_index] & 1))
    //                 continue;
    //             new_pt[pt_index] = old_pt[pt_index];
    //         }
    //     }
    // }

    // Remap the stack
    remap_stack(old_pml4);

    kprintf("Stack Working!!");

    return;

}






// // TODO: Solve with malloc
// void preserve_limine_requests() {
//     // Allocate memory for each Limine request structure
//     uint64_t hhdm_phys = pmm_alloc();
//     uint64_t exec_phys = pmm_alloc();
//     uint64_t fb_phys = pmm_alloc();

//     uint64_t hddm_response_phys = pmm_alloc();
//     uint64_t exec_response_phys = pmm_alloc();
//     uint64_t fb_response_phys = pmm_alloc();
//     uint64_t framebuffer_struct_phys = pmm_alloc();


//     if (!hhdm_phys || !exec_phys || !fb_phys || !hddm_response_phys || !exec_response_phys) {
//         // Handle memory allocation failure (e.g., panic, log error)
//         return;
//     }

//     // Create response structures
//     struct limine_hhdm_response *hhdm_response = (void *)PHYS_TO_VIRT(hddm_response_phys);
//     struct limine_kernel_file_response *exec_response = (void *)PHYS_TO_VIRT(exec_response_phys);
//     struct limine_framebuffer_response *fb_response = (void *)PHYS_TO_VIRT(fb_response_phys);
//     struct limine_framebuffer *framebuffer_struct = (void *)PHYS_TO_VIRT(framebuffer_struct_phys);

//     // Compute virtual addresses using HHDM
//     volatile struct limine_hhdm_request *new_hhdm_request = (void *)(hhdm_phys + HHDM_OFFSET);
//     volatile struct limine_kernel_file_request *new_exec_file = (void *)(exec_phys + HHDM_OFFSET);
//     volatile struct limine_framebuffer_request *new_framebuffer_request = (void *)(fb_phys + HHDM_OFFSET);


//     // Copy original structures to allocated memory
//     memcpy((void *)new_hhdm_request, (void *)&hhdm_request, sizeof(hhdm_request));
//     memcpy((void *)new_exec_file, (void *)&exec_file, sizeof(exec_file));
//     memcpy((void *)new_framebuffer_request, (void *)&framebuffer_request, sizeof(framebuffer_request));
    
//     memcpy((void*)hhdm_response, (void*)hhdm_request.response, sizeof(struct limine_hhdm_response));
//     memcpy((void*)exec_response, (void*)exec_file.response, sizeof(struct limine_kernel_file_response));
//     memcpy((void*)fb_response, (void*)framebuffer_request.response, sizeof(struct limine_framebuffer_response));

//     memcpy((void*)framebuffer_struct, (void*)framebuffer_request.response->framebuffers[0], sizeof(struct limine_framebuffer));
    
//     // Update pointers to response structures
//     new_hhdm_request->response = hhdm_response;
//     new_exec_file->response = exec_response;
//     new_framebuffer_request->response = fb_response;

//     new_framebuffer_request->response->framebuffers[0] = framebuffer_struct;

//     // Update pointers to new structures
//     hhdm_request = *new_hhdm_request;
//     exec_file = *new_exec_file;
//     framebuffer_request = *new_framebuffer_request;


    
// }

