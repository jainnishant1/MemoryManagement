#ifndef HEAP_H
#define HEAP_H

// void hi();

typedef struct _list_node{
    struct _list_node *next;
    struct _list_node *prev;
}list_node_t;

typedef struct _list{
    list_node_t *head;
    unsigned int offset;
}list_t;

//MACRO to Iterate over 

void list_node_insert(list_t *head,list_node_t *node);

void list_node_delete(list_t *head, list_node_t *node);

#endif