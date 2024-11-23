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


// Sort the list descending using counting sort
// The key of a node is given by the get_key function
void sort_list(List *list, int (*get_key)(void*))
{   
    if (!list || !list->size)
        return;

    int max_value = 0;
    for (ListNode *node = list->head; node; node = node->next)
        max_value = MAX(max_value, get_key(node));

    int *freq = calloc(max_value + 1, sizeof(*freq));
    CHECK_MALLOC(freq);

    for (ListNode *node = list->head; node; node = node->next)
        freq[get_key(node)]++;

    for (int i = 1; i <= max_value; i++)
        freq[i] += freq[i - 1];

    ListNode **sorted_list = calloc(list->size, sizeof(*sorted_list));
    CHECK_MALLOC(sorted_list);

    for (ListNode *node = list->head; node; node = node->next) {
        int ones = get_key(node);
        sorted_list[freq[ones] - 1] = node;
        freq[ones]--;
    }

    list->head = sorted_list[list->size - 1];
    sorted_list[0]->next = NULL;
    for (int i = list->size - 2; i >= 0; i--)
        sorted_list[i + 1]->next = sorted_list[i];

    free(freq);
    free(sorted_list);
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
