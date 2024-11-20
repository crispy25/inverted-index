#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "utils.h"

List *create_list()
{
    List *list = calloc(1, sizeof(*list));
    CHECK_MALLOC(list);

    return list;
}


void push_front(List *list, void *data)
{
    ListNode *node = calloc(1, sizeof(*node));
    CHECK_MALLOC(node);
    node->data = data;

    if (!list->size) {
        list->head = node;
        list->tail = node;
        list->size++;
        return;

    }

    node->next = list->head;
    list->head = node;
    list->size++;
}


void push_back(List *list, void *data)
{
    ListNode *node = calloc(1, sizeof(*node));
    CHECK_MALLOC(node);
    node->data = data;

    if (!list->size) {
        list->head = node;
        list->tail = node;
        list->size++;
        return;
    }

    list->tail->next = node;
    list->tail = node;
    list->size++;
}


void sort_list(List *list, int (*comp)(const void*, const void*))
{
    if (!list || !list->head || list->size == 1)
        return;

    int len = list->size, i = 0;
    ListNode **nodes = calloc(len, sizeof(*nodes));
    
    for (ListNode* node = list->head; node; node = node->next)
        nodes[i++] = node;

    // qsort(nodes, len, sizeof(list->head), comp);
    for (int i = 1; i < len; i++) {
        void *value = nodes[i];
        int pos = i - 1;

        while (pos >= 0 && comp(nodes[pos], value) > 0) {
            nodes[pos + 1] =  nodes[pos];
            pos -= 1;
        }
        nodes[pos + 1] = value;
    }

    nodes[len - 1]->next = NULL;
    list->head = nodes[0];
    ListNode *prev = list->head;
    for (i = 1; i < len; i++) {
        prev->next = nodes[i];
        prev = prev->next;
    }

    free(nodes);
}


void free_nodes(List *list, void (*free_func)(void*))
{   
    ListNode *node = list->head, *next;
    while (node) {
        next = node->next;
        free_func(node);
        node = next;
    }
}


void free_list(List **list, void (*free_func)(void*))
{   
    ListNode *node = (*list)->head, *next;
    while (node) {
        next = node->next;
        free_func(node);
        node = next;
    }
    
    free(*list);
}
