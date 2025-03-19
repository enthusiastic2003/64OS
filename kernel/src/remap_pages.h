#pragma once

#ifndef REMAP_PAGES_H
#define REMAP_PAGES_H
#include<stdint.h>
#include "limine_requests.h"
#include "limine.h"


extern uint64_t _end;

#define LAST_HIGH_MEM_ADDRESS (uint64_t)&_end
#define PAGE_SIZE 4096




void remap_kernel();

#endif

