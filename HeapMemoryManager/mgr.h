#ifndef MGR_H
#define MGR_H

#include <stdio.h>                      // for size_t
#include "heap.h"
#define MAX_STRUCT_NAME_LEN 32

typedef enum { MGR_TRUE, MGR_FALSE } mgr_bool_t;
extern size_t VIRTUAL_PAGE_SIZE;

#define MAX_FAMILIES_PER_VPAGE (VIRTUAL_PAGE_SIZE - sizeof(families_vpage_t*)) / sizeof(vpage_family_t)

typedef struct _meta_block {
    size_t data_block_size;
    size_t offset;
    mgr_bool_t is_free;
    heap_node_t* heap_ptr;
    struct _meta_block *prev, *next;
} meta_block_t;

//Forward declarations to be used in struct  _vpage
struct _vpage_family;

typedef struct _vpage
{
    struct _vpage *next, *prev;
    struct _vpage_family *page_family;
    struct _meta_block meta_block;
    char pages[];
} vpage_t;

typedef struct _vpage_family
{
    char struct_name[MAX_STRUCT_NAME_LEN];
    size_t struct_size; // uint32_t check
    vpage_t *first;
    heap_t heap_head;
} vpage_family_t;

typedef struct _families_vpage
{
    struct _families_vpage *next;
    vpage_family_t vpage_family[]; // FAM check, standardized in C99
} families_vpage_t;

#define FIELD_OFFSET(struct_name, field_name) (size_t)&(((struct_name*)0)->field_name)

#define META_BLOCK_PAGE(meta_block_ptr) ((void*)((char*) meta_block_ptr - meta_block_ptr->offset))

#define NEXT_META_BLOCK_BY_SIZE(meta_block_ptr) \
        (meta_block_t *)((char*)(meta_block_ptr + 1) + meta_block_ptr->data_block_size)

#define MGR_ALLOCATION_PREV_NEXT_UPDATE(meta_block_alloc_ptr, meta_block_free_ptr)           \
    meta_block_free_ptr->prev = meta_block_alloc_ptr;             \
    meta_block_free_ptr->next = meta_block_alloc_ptr->next; \
    meta_block_alloc_ptr->next = meta_block_free_ptr;             \
    if (meta_block_free_ptr->next)                                \
    meta_block_free_ptr->next->prev = meta_block_free_ptr

#define ITER_VPAGE_FAMILIES_BEGIN(families_vpage_ptr, cur_vpage_family) \
{                                                                    \
    size_t index = 0;                                                \
    for(cur_vpage_family = (vpage_family_t*)&(families_vpage_ptr->vpage_family[0]); \
        cur_vpage_family->struct_size && index < MAX_FAMILIES_PER_VPAGE; index++, cur_vpage_family++) \
    {               

#define ITER_VPAGE_FAMILIES_END }}

#define ITER_VPAGE_BEGIN(vpage_family_ptr,cur_vpage) \
{                                                                               \
    vpage_t *next = NULL;                                                       \
    for(cur_vpage = vpage_family_ptr->first;cur_vpage;cur_vpage=next){    \
        next = cur_vpage->next;

#define ITER_VPAGE_END(vpage_family_ptr,cur_vpage) }}

#define ITER_VPAGE_META_BLOCKS_BEGIN(vpage_ptr,cur_meta_block) \
{                                                                                      \
    meta_block_t *next = NULL;                                                         \
    for(cur_meta_block = &vpage_ptr->meta_block;cur_meta_block;cur_meta_block = next){ \
        next = cur_meta_block->next;

#define ITER_VPAGE_META_BLOCKS_END(vpage_ptr, cur_meta_block) }}

#define VPAGE_MARK_EMPTY(vpage_ptr)                                 \
    vpage_ptr->meta_block.next = vpage_ptr->meta_block.prev = NULL; \
    vpage_ptr->meta_block.is_free = MGR_TRUE;

void* mgr_allocate_vm_pages(int count);

void mgr_free_vm_pages(void* pages, int count);

mgr_bool_t mgr_is_vpage_empty(vpage_t *vpage_ptr);

vpage_t *request_new_data_vpage(vpage_family_t *vpage_family_ptr);

void mgr_vpage_free_delete(vpage_t *vpage_ptr);

#endif
