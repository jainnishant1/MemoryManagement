#ifndef MEMMGR_H
#define MEMMGR_H

#include <stdio.h>
#include "mgr.h"
#include "heap.h"

void mgr_init();

void mgr_register_page_family(char* struct_name, size_t struct_size);

#define MGR_STRUCT_REG(struct_name) mgr_register_page_family(#struct_name, sizeof(struct_name))

void mgr_print_reg_page_families();
vpage_family_t* mgr_lookup_page_family(char* struct_name);

#endif