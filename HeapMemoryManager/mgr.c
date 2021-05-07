#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>     // only for exit()
#include "mgr.h"

static families_vpage_t* first_families_vpage = NULL;
size_t VIRTUAL_PAGE_SIZE = 0;

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
    // cur_vpage_family->first_page = NULL;     // check
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

mgr_bool_t mgr_is_vpage_empty(vpage_t *vpage_ptr)
{
    if(vpage_ptr->meta_block.prev==NULL && vpage_ptr->meta_block.next==NULL && vpage_ptr->meta_block.is_free==MGR_TRUE){
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

    new_page->meta_block.data_block_size = mgr_size_free_data_block_vpage(1);
    new_page->meta_block.offset = FIELD_OFFSET(vpage_t,meta_block);

    new_page->next = new_page->prev = NULL;

    new_page->page_family= vpage_family_ptr;

    if(!vpage_family_ptr->first){
        vpage_family_ptr->first = new_page;
        return new_page;
    }

    new_page->next = vpage_family_ptr->first;
    vpage_family_ptr->first->prev = new_page;
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

    if(vpage_ptr->next){
        vpage_ptr->next->prev = vpage_ptr->prev;
    }

    vpage_ptr->prev->next = vpage_ptr->next;
    mgr_free_vm_pages((void *)vpage_ptr,1);
}