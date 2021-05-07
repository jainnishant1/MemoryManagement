#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>     // only for exit()
#include "mgr.h"

static families_vpage_t* first_families_vpage = NULL;
size_t VIRTUAL_PAGE_SIZE = 0;

int comparator_for_heap(void *a,void *b){
    meta_block_t *first = (meta_block_t *)a;
    meta_block_t *second = (meta_block_t *)b;
    if(first->data_block_size>second->data_block_size){
        return -1;
    }
    else if (first->data_block_size == second->data_block_size){
        return 0;
    }
    return 1;
}

void mgr_init() {
    VIRTUAL_PAGE_SIZE = getpagesize();
    printf("Virtual page size = %lu bytes\n", VIRTUAL_PAGE_SIZE);
}

void* mgr_allocate_vm_pages(int count) {
    size_t nbytes = count * VIRTUAL_PAGE_SIZE;
    char* pages = mmap(0, nbytes,
                       PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_ANON | MAP_PRIVATE, 0, 0);
    if(pages == MAP_FAILED) {
        fprintf(stderr, "Error: unable to allocate the desired virtual pages\n");
        return NULL;
    }
    memset(pages, 0, nbytes);
    return (void*) pages;
}

void mgr_free_vm_pages(void* pages, int count) {
    int rval = munmap(pages, count * VIRTUAL_PAGE_SIZE);
    if(rval) fprintf(stderr, "Error: unable to free the desired virtual pages\n");
}

void mgr_register_page_family(char* struct_name, size_t struct_size) {
    if(struct_size > VIRTUAL_PAGE_SIZE) {
        fprintf(stderr, "Error: size of struct %s being registered exceeds page size\n", struct_name);
        return;
    }

    vpage_family_t* cur_vpage_family = NULL;
    families_vpage_t* new_families_vpage = NULL;

    if(first_families_vpage == NULL) {
        // first page family being registered
        first_families_vpage = (families_vpage_t*) mgr_allocate_vm_pages(1);
        first_families_vpage->next = NULL;
        first_families_vpage->vpage_family[0].struct_size = struct_size;
        strncpy(first_families_vpage->vpage_family[0].struct_name, struct_name, MAX_STRUCT_NAME_LEN);
        first_families_vpage->vpage_family[0].first=NULL;
        init_heap(&first_families_vpage->vpage_family[0].heap_head, &comparator_for_heap);
        return;
    }

    size_t index = 0;

    ITER_VPAGE_FAMILIES_BEGIN(first_families_vpage, cur_vpage_family) {
        if(strncmp(cur_vpage_family->struct_name, struct_name, MAX_STRUCT_NAME_LEN) == 0) {
            fprintf(stderr, "Error: re-registration of struct %s\n", struct_name);
            exit(1);
        }
        index++;
    } ITER_VPAGE_FAMILIES_END;

    if(index == MAX_FAMILIES_PER_VPAGE) {
        // first virtual page for page families is full
        new_families_vpage = (families_vpage_t*) mgr_allocate_vm_pages(1);
        new_families_vpage->next = first_families_vpage;
        first_families_vpage = new_families_vpage;
        cur_vpage_family = &(first_families_vpage->vpage_family[0]);
    }
    // cur_vpage_family points to the first empty location in the correct page for families
    cur_vpage_family->struct_size = struct_size;
    strncpy(cur_vpage_family->struct_name, struct_name, MAX_STRUCT_NAME_LEN);
    cur_vpage_family->first = NULL;
    init_heap(&cur_vpage_family->heap_head, &comparator_for_heap);
}

void mgr_print_reg_page_families() {
    families_vpage_t* cur_families_vpage = first_families_vpage;
    vpage_family_t* cur_vpage_family = NULL;
    size_t count = 0;
    while(cur_families_vpage) {

        ITER_VPAGE_FAMILIES_BEGIN(cur_families_vpage, cur_vpage_family) {
            printf("Page family #%lu: Name = %s, Size = %lu\n",
                    count,
                    cur_vpage_family->struct_name,
                    cur_vpage_family->struct_size);
            count++;
        } ITER_VPAGE_FAMILIES_END;
        cur_families_vpage = cur_families_vpage->next;
    }
}

vpage_family_t* mgr_lookup_page_family(char* struct_name) {
    families_vpage_t* cur_families_vpage = first_families_vpage;
    vpage_family_t* cur_vpage_family = NULL;

    while(cur_families_vpage) {

        ITER_VPAGE_FAMILIES_BEGIN(cur_families_vpage, cur_vpage_family) {
            if(strncmp(cur_vpage_family->struct_name, struct_name, MAX_STRUCT_NAME_LEN) == 0) {
                return cur_vpage_family;
            }
        } ITER_VPAGE_FAMILIES_END;
        cur_families_vpage = cur_families_vpage->next;
    }

    return NULL;
}

static void mgr_merge_free_block(meta_block_t *first, meta_block_t *second){
    if(first->is_free==MGR_FALSE||second->is_free==MGR_FALSE){
        fprintf(stderr, "Error: Merging of Allocated blocks is not allowed\n");
        exit(1);
    }

    first->data_block_size+= second->data_block_size + sizeof(meta_block_t);

    first->next = second->next;

    if(second->next){
        second->next->prev = first;
    }
}

static void mgr_insert_vpage_family_free_block_in_heap(vpage_family_t *page_family_ptr,meta_block_t *free_block_ptr){
    if(free_block_ptr->is_free==MGR_FALSE){
        fprintf(stderr, "Error: Only free blocks can be inserted in heap\n");
        exit(1);
    }
    heap_insert(&page_family_ptr->heap_head,(void *)free_block_ptr);
}

mgr_bool_t mgr_is_vpage_empty(vpage_t *vpage_ptr)
{
    if (vpage_ptr->meta_block.prev == NULL && vpage_ptr->meta_block.is_free == MGR_TRUE && vpage_ptr->meta_block.next == NULL)
    {
        return MGR_TRUE;
    }
    return MGR_FALSE;
}

static inline size_t mgr_size_free_data_block_vpage(int count){

    return (size_t)((VIRTUAL_PAGE_SIZE * count) - FIELD_OFFSET(vpage_t,pages));
}

vpage_t *request_new_data_vpage(vpage_family_t *vpage_family_ptr){
    vpage_t *new_page = mgr_allocate_vm_pages(1);

    VPAGE_MARK_EMPTY(new_page);
    
    new_page->next = new_page->prev = NULL;

    new_page->meta_block.data_block_size = mgr_size_free_data_block_vpage(1);
    new_page->meta_block.offset = FIELD_OFFSET(vpage_t,meta_block);
    new_page->meta_block.heap_ptr=NULL;


    new_page->page_family= vpage_family_ptr;

    if(vpage_family_ptr->first==NULL){
        vpage_family_ptr->first = new_page;
        return new_page;
    }

    vpage_family_ptr->first->prev = new_page;
    new_page->next = vpage_family_ptr->first;
    vpage_family_ptr->first = new_page;
    return new_page;
}

void mgr_vpage_free_delete(vpage_t *vpage_ptr){

    if(vpage_ptr->page_family->first==vpage_ptr){
        vpage_ptr->page_family->first = vpage_ptr->next;
        if(vpage_ptr->next){
            vpage_ptr->next->prev = NULL;
        }
        vpage_ptr->next = vpage_ptr->prev=NULL;
        mgr_free_vm_pages((void *)vpage_ptr,1);
        return;
    }

    if(vpage_ptr->next!=NULL){
        vpage_ptr->next->prev = vpage_ptr->prev;
    }

    vpage_ptr->prev->next = vpage_ptr->next;
    mgr_free_vm_pages((void *)vpage_ptr,1);
}

static mgr_bool_t mgr_split_and_allocate(vpage_family_t *vpage_family_ptr,meta_block_t *free_block, size_t size){
    if(free_block->is_free==MGR_FALSE){
        fprintf(stderr, "Error: Only free blocks can be splitted\n");
        exit(1);
    }

    if(free_block->data_block_size<size){
        return MGR_FALSE;
    }

    size_t remain = free_block->data_block_size - size;
    free_block->is_free = MGR_FALSE;
    free_block->data_block_size = size;

    heap_remove(&vpage_family_ptr->heap_head,(void *)free_block);

    //CASE 1: remain = 0;
    if(!remain){
        return MGR_TRUE;
    }

    //Case 2: Soft Internal Fragmentation
    if((remain>sizeof(meta_block_t)&&remain<(sizeof(meta_block_t)+vpage_family_ptr->struct_size))||remain>=sizeof(meta_block_t)){
        meta_block_t *next_free_block = NEXT_META_BLOCK_BY_SIZE(free_block);
        next_free_block->data_block_size = remain - sizeof(meta_block_t);
        next_free_block->is_free = MGR_TRUE;
        next_free_block->offset = free_block->offset + sizeof(meta_block_t)+free_block->data_block_size;

        next_free_block->heap_ptr = NULL; //check?????
        mgr_insert_vpage_family_free_block_in_heap(vpage_family_ptr,next_free_block);
        
        MGR_ALLOCATION_PREV_NEXT_UPDATE(free_block,next_free_block);
    }

    //Case 3: Hard Internal Fragmentation
    else if(remain<sizeof(meta_block_t)){
        //Nothing
    }

    // else{
    //     meta_block_t *next_free_block = NEXT_META_BLOCK_BY_SIZE(free_block);
    //     next_free_block->is_free = MGR_TRUE;
    //     next_free_block->data_block_size = remain - sizeof(meta_block_t);
    //     next_free_block->offset = free_block->offset + sizeof(meta_block_t) + free_block->data_block_size;

    //     next_free_block->heap_ptr = NULL; //check?????
    //     mgr_insert_vpage_family_free_block_in_heap(vpage_family_ptr, next_free_block);

    //     MGR_ALLOCATION_PREV_NEXT_UPDATE(free_block, next_free_block);
    // }

    return MGR_TRUE;
}

static meta_block_t *mgr_allocate_free_block(vpage_family_t *vpage_family_ptr,size_t size){

    meta_block_t *largest = (meta_block_t *)heap_front(&vpage_family_ptr->heap_head);


    if(largest==NULL||largest->data_block_size<size){
        vpage_t *new_page = request_new_data_vpage(vpage_family_ptr);

        mgr_insert_vpage_family_free_block_in_heap(vpage_family_ptr,&new_page->meta_block);

        if(mgr_split_and_allocate(vpage_family_ptr,&new_page->meta_block,size)==MGR_TRUE){
            return &new_page->meta_block;
        }
        return NULL;
    }

    if(largest){
        if (mgr_split_and_allocate(vpage_family_ptr, largest, size) == MGR_TRUE){
            return largest;
        }
    }

    return NULL;

}

void *xcalloc(char *struct_name,int count){

    vpage_family_t *vpage_family_ptr = mgr_lookup_page_family(struct_name);

    if(vpage_family_ptr==NULL){
        fprintf(stderr, "Error: Structure %s is not registered\n",struct_name);
        return NULL;
    }

    if(vpage_family_ptr->struct_size * count > mgr_size_free_data_block_vpage(1)){
        fprintf(stderr, "Error: Memory Requested Exceeds Page size\n");
        return NULL;
    }

    meta_block_t *free_block = mgr_allocate_free_block(vpage_family_ptr, vpage_family_ptr->struct_size * count);

    if(free_block!=NULL){
        memset((char *)(free_block+1),0,free_block->data_block_size);
        return (void *)(free_block+1);
    }

    return NULL;
}

static void mgr_free_allocated_data_block(meta_block_t *allocated_block_ptr){

    vpage_t *vpage = (vpage_t *)META_BLOCK_PAGE(allocated_block_ptr);

    meta_block_t *priority_queue_block = NULL;

    allocated_block_ptr->is_free = MGR_TRUE;

    priority_queue_block = allocated_block_ptr;

    if(allocated_block_ptr->next==NULL){
        char *first = (char *)(allocated_block_ptr+1) + allocated_block_ptr->data_block_size;
        char *second = (char *)((char *)vpage + VIRTUAL_PAGE_SIZE);

        allocated_block_ptr->data_block_size += ((int)((unsigned long)second - (unsigned long)first));
    }
    else{
        meta_block_t *next_meta_block = NEXT_META_BLOCK_BY_SIZE(allocated_block_ptr);
        int hard_fragmented_size = (int)((unsigned long)allocated_block_ptr->next -(unsigned long)next_meta_block);

        allocated_block_ptr->data_block_size +=hard_fragmented_size;
    }

    if(allocated_block_ptr->next!=NULL && allocated_block_ptr->next->is_free==MGR_TRUE){
        mgr_merge_free_block(allocated_block_ptr,allocated_block_ptr->next);
        priority_queue_block = allocated_block_ptr;
    }
    if (allocated_block_ptr->prev != NULL && allocated_block_ptr->prev->is_free == MGR_TRUE){
        mgr_merge_free_block(allocated_block_ptr->prev,allocated_block_ptr);
        priority_queue_block = allocated_block_ptr->prev;
    }

    if(mgr_is_vpage_empty(vpage)==MGR_TRUE){
        mgr_vpage_free_delete(vpage);
        return;
    }

    heap_insert(&vpage->page_family->heap_head,(void *)priority_queue_block);

}

void *xfree(void *data_block_ptr){
    meta_block_t *meta_block_ptr = (meta_block_t *)((char *)data_block_ptr - sizeof(meta_block_t));

    if(meta_block_ptr->is_free==MGR_TRUE){
        fprintf(stderr, "Error: Only allocated blocks can be freed\n");
        return NULL;
    }

    mgr_free_allocated_data_block(meta_block_ptr);
}