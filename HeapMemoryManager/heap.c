#include <stdio.h>
#include "mgr.h"
#include "heap.h"

void swap_ptr(void** ptr1, void** ptr2) {
    void* tmp = *ptr1;
    *ptr1 = *ptr2;
    *ptr2 = tmp;
}

void init_heap(heap_t* heap, heap_cmp_f cmp) {
    heap->first_incomplete_index = 0;
    heap->first_incomplete_vpage = NULL;
    heap->last = heap->first = NULL;
    heap->heap_cmp = cmp;
}

heap_vpage_t* allocate_heap_vpage(heap_t* heap) {
    heap_vpage_t* heap_vpage = (heap_vpage_t*) mgr_allocate_vm_pages(1);
    heap_vpage->size = 0;
    heap_vpage->next = heap->last;
    heap_vpage->prev = NULL;
    if(heap->last) heap->last->prev = heap_vpage;
    heap->last = heap_vpage;
    return heap_vpage;
}

void free_heap_vpage(heap_t* heap) {
    if(heap->last == 0) return;
    if(heap->last->next) heap->last->next->prev = NULL;
    heap_vpage_t* next = heap->last->next;
    mgr_free_vm_pages((void*)(heap->last), 1);
    heap->last = next;
}

int heap_empty(heap_t* heap) {
    return heap == 0 || heap->first == 0 || heap->first->size == 0;
}

heap_node_t* new_heap_node(heap_t* heap, void* data) {
    if(heap->last == 0) {
        allocate_heap_vpage(heap);
        heap->last->size++;
        heap->last->nodes[0].data = data;
        heap->last->nodes[0].left = heap->last->nodes[0].right = NULL;
        heap->last->nodes[0].parent = NULL;
        heap->first_incomplete_vpage = heap->last;
        heap->first_incomplete_index = 0;
        heap->first = heap->last;
        ((heap_data_t*) data)->heap_ptr = &(heap->last->nodes[0]);
        return &(heap->last->nodes[0]);
    }
    if(heap->last->size == MAX_NODES_PER_VPAGE) {
        allocate_heap_vpage(heap);
    }
    heap->last->nodes[heap->last->size].data = data;
    heap->last->nodes[heap->last->size].left = heap->last->nodes[heap->last->size].right = NULL;
    heap_node_t* first_incomplete_node = FIRST_INCOMPLETE_NODE(heap);
    heap->last->nodes[heap->last->size].parent = first_incomplete_node;

    if(first_incomplete_node->left == 0)
        first_incomplete_node->left = &(heap->last->nodes[heap->last->size]);
    else {
        first_incomplete_node->right = &(heap->last->nodes[heap->last->size]);
        heap->first_incomplete_index++;
        if(heap->first_incomplete_index == MAX_NODES_PER_VPAGE) {
            heap->first_incomplete_vpage = heap->first_incomplete_vpage->prev;
            heap->first_incomplete_index = 0;
        }
    }
    ((heap_data_t*) data)->heap_ptr = &(heap->last->nodes[heap->last->size]);
    heap->last->size++;
    return &(heap->last->nodes[heap->last->size-1]);
}

void delete_last_heap_node(heap_t* heap) {
    if(heap_empty(heap)) return;
    heap_node_t* parent = heap->last->nodes[heap->last->size-1].parent;
    if(parent) {
        if(parent->right) {
            parent->right = NULL;
            if(heap->first_incomplete_index == 0) {
                heap->first_incomplete_index = MAX_NODES_PER_VPAGE - 1;
                heap->first_incomplete_vpage = heap->first_incomplete_vpage->next;
            } else {
                heap->first_incomplete_index--;
            }
        } else parent->left = NULL;
    }
    ((heap_data_t*) (heap->last->nodes[heap->last->size-1].data))->heap_ptr = NULL;
    heap->last->size--;
    if(heap->last->size == 0) {
        
        if(heap->last->next == 0) heap->first = NULL;
        free_heap_vpage(heap);
    }
}

void heapify(heap_t* heap, heap_node_t* node) {
    heap_node_t* targ_node = node;
    if(node->left && heap->heap_cmp(targ_node->data, node->left->data) == 1) {
        targ_node = node->left;
    } 
    if(node->right && heap->heap_cmp(targ_node->data, node->right->data) == 1) {
        targ_node = node->right;
    }
    if(targ_node != node) {
        swap_ptr(&(node->data), &(targ_node->data));
        ((heap_data_t*) (node->data))->heap_ptr = node;
        ((heap_data_t*) (targ_node->data))->heap_ptr = targ_node;
        heapify(heap, targ_node);
    }
}

void percolate_up(heap_t* heap, heap_node_t* node) {
    while(node->parent && heap->heap_cmp(node->parent->data, node->data) == 1) {
        swap_ptr(&(node->parent->data), &(node->data));
        ((heap_data_t*) (node->data))->heap_ptr = node;
        ((heap_data_t*) (node->parent->data))->heap_ptr = node->parent;
        node = node->parent;
    }
}

void heap_insert(heap_t* heap, void* data) {
    heap_node_t* heap_node = new_heap_node(heap, data);
    percolate_up(heap, heap_node);
}

void* heap_pop(heap_t* heap) {
    if(heap_empty(heap)) return NULL;
    void* data = heap->first->nodes[0].data;
    ((heap_data_t*) data)->heap_ptr = NULL;
    heap->first->nodes[0].data = heap->last->nodes[heap->last->size-1].data;
    delete_last_heap_node(heap);
    if(heap->first) {
        ((heap_data_t*) (heap->first->nodes[0].data))->heap_ptr = &(heap->first->nodes[0]);
        heapify(heap, &(heap->first->nodes[0]));
    }
    return data;
}

void* heap_front(heap_t* heap) {
    if(heap_empty(heap)) return NULL;
    return heap->first->nodes[0].data;
}

void heap_remove_node(heap_t* heap, heap_node_t* node) {
    ((heap_data_t*) (node->data))->heap_ptr = NULL;
    node->data = heap->last->nodes[heap->last->size-1].data;
    
    if(node == &(heap->last->nodes[heap->last->size-1])) {
        delete_last_heap_node(heap);
    } else {
        delete_last_heap_node(heap);
        ((heap_data_t*) (node->data))->heap_ptr = node;
        if(node->parent && heap->heap_cmp(node->parent->data, node->data) == 1)
            percolate_up(heap, node);
        else heapify(heap, node);
    }
}

void heap_remove(heap_t* heap, void* data) {
    heap_node_t* node = ((heap_data_t*) data)->heap_ptr;
    if(node) heap_remove_node(heap, node);
}

