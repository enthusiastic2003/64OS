//
// Created by sirjanh on 6/7/25.
//

#ifndef VMM_UTILS_H
#define VMM_UTILS_H

#include "pmm_mngr.h"
#include "vmm_mngr.h"

phys_addr_t create_page_table(); // Done
void free_page_table(phys_addr_t pt); // Done

int map_page(virt_addr_t va, phys_addr_t pa, uint64_t flags);// Done
void unmap_page(virt_addr_t va);//Done
phys_addr_t get_physical_address(virt_addr_t va);// Done
void set_page_flags(virt_addr_t va, uint64_t flags);

void load_cr3(phys_addr_t pml4_addr);
void flush_tlb();
void flush_tlb_single(virt_addr_t va);
int is_page_present(virt_addr_t va);

pml4_t* get_pml4_virtual_address(void);//dome
pdpt_t* get_pdpt_virtual_address(int pdpt_index);//done//done
pd_t* get_pd_virtual_address(int pdpt_index, int pd_index);//done
pt_t* get_pt_virtual_address(int pdpt_index, int pd_index, int pt_index);//done
#endif //VMM_UTILS_H
