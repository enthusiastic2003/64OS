//
// Created by sirjanh on 6/7/25.
//

#ifndef PRINTING_UTILS_H
#define PRINTING_UTILS_H
#include <stdint.h>
void print_page_table_level(void* table_virt_addr, int level, uint64_t hhdm_base);
void print_page_table(uint64_t virt_pml4_addr, uint64_t hhdm_base);
void print_indent(int level);
void print_page_table_level(void* table_virt_addr, int level, uint64_t hhdm_base);

#endif //PRINTING_UTILS_H
