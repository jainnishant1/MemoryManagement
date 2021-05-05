#include <stdio.h>
#include <memory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>     // only for exit()
#include "mgr.h"

static families_vpage_t* first_families_vpage = NULL;
static size_t VIRTUAL_PAGE_SIZE = 0;

void mgr_init() {
    VIRTUAL_PAGE_SIZE = getpagesize();
    printf("Virtual page size = %lu bytes\n", VIRTUAL_PAGE_SIZE);
}

static void* mgr_allocate_vm_pages(int count) {
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

static void mgr_free_vm_pages(void* pages, int count) {
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