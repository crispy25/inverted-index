#ifndef _LIST_H_
#define _LIST_H_

#include <stdlib.h>


typedef struct ListNode {
    struct ListNode *next;
    void *data;
} ListNode;


typedef struct List {
    ListNode *head;
    ListNode *tail;
    size_t size;
} List;


List *create_list();

void push_front(List *list, void *data);

void push_back(List *list, void *data);

void sort_list(List *list, int (*get_key)(void *));

void free_nodes(List *list, void (*free_func)(void*));

void free_list(List **list, void (*free_func)(void*));


#endif
