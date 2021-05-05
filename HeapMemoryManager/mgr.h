#ifndef MGR_H
#define MGR_H

#include <stdio.h>                      // for size_t
#define MAX_STRUCT_NAME_LEN 32

typedef enum { MGR_TRUE, MGR_FALSE } mgr_bool_t;

typedef struct _vpage_family {
    char struct_name[MAX_STRUCT_NAME_LEN];
    size_t struct_size;                 // uint32_t check
} vpage_family_t;

typedef struct _families_vpage {
    struct _families_vpage* next;
    vpage_family_t vpage_family[];      // FAM check, standardized in C99
} families_vpage_t;

#define MAX_FAMILIES_PER_VPAGE (VIRTUAL_PAGE_SIZE - sizeof(families_vpage_t*)) / sizeof(vpage_family_t)

typedef struct _meta_block {
    size_t data_block_size;
    size_t offset;
    mgr_bool_t is_free;
    struct _meta_block *prev, *next;
} meta_block_t;

#define FIELD_OFFSET(struct_name, field_name) (int)&(((struct_name*)0)->field_name)

#define META_BLOCK_PAGE(meta_block_ptr) (void*)((char*) meta_block_ptr - meta_block_ptr->offset)

#define NEXT_META_BLOCK_BY_SIZE(meta_block_ptr) \
        (meta_block_ptr*)((char*)(meta_block_ptr + 1) + meta_block_ptr->block_size)

#define ITER_VPAGE_FAMILIES_BEGIN(families_vpage_ptr, cur_vpage_family) \
{                                                                    \
    size_t index = 0;                                                \
    for(cur_vpage_family = (vpage_family_t*)&(families_vpage_ptr->vpage_family[0]); \
        cur_vpage_family->struct_size && index < MAX_FAMILIES_PER_VPAGE; index++, cur_vpage_family++) \
    {               

#define ITER_VPAGE_FAMILIES_END }}

#endif
