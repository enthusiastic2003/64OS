#include<stdint.h>
#include "page_fault_handler.h"

extern void page_fault_handler(struct interrupt_frame*, uint64_t);