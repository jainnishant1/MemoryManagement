#ifndef MEMMGR_H
#define MEMMGR_H

#include <stdio.h>
#include "mgr.h"
#include "heap.h"

void mgr_init();

void *xmalloc(char *struct_name, int count);
void xfree(void *ptr);

void mgr_register_page_family(char* struct_name, size_t struct_size);

#define MGR_STRUCT_REG(struct_name) mgr_register_page_family(#struct_name, sizeof(struct_name))

void mgr_print_reg_page_families();
#define XMALLOC(struct_name,units) \
    (xmalloc(#struct_name, units))
#define XFREE(ptr) \
    (xfree(ptr))
vpage_family_t* mgr_lookup_page_family(char* struct_name);

void mm_print_memory_usage(char *struct_name);
void mm_print_registered_page_families();
void mm_print_block_usage();
void mm_print_total_fragmented_size();

#endif