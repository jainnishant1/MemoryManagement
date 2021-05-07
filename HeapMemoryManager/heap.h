#ifndef HEAP_H
#define HEAP_H

#include <stdio.h>
#include "mgr.h"

struct _heap_node;
struct _heap_vpage;

// typedef struct {
//     int a;
//     struct _heap_node* heap_ptr;
// } heap_data_t;
typedef struct _meta_block heap_data_t;

typedef int (*heap_cmp_f)(void*, void*);

typedef struct _heap {
    struct _heap_vpage* first_incomplete_vpage;
    size_t first_incomplete_index;
    struct _heap_vpage *first, *last;
    heap_cmp_f heap_cmp;
} heap_t;

typedef struct _heap_node {
    void* data;
    struct _heap_node *left, *right, *parent;
} heap_node_t;

typedef struct _heap_vpage {
    size_t size;
    struct _heap_vpage *next, *prev;
    heap_node_t nodes[];
} heap_vpage_t;

#define MAX_NODES_PER_VPAGE (VIRTUAL_PAGE_SIZE - sizeof(size_t) - sizeof(heap_vpage_t*)) / sizeof(heap_node_t)
#define FIRST_INCOMPLETE_NODE(heap) \
        &(heap->first_incomplete_vpage->nodes[heap->first_incomplete_index])

void init_heap(heap_t* heap, heap_cmp_f cmp);

int heap_empty(heap_t* heap);

void heap_insert(heap_t* heap, void* data);

void* heap_pop(heap_t* heap);

void* heap_front(heap_t* heap);

void heap_remove(heap_t* heap, void* data);

#endif