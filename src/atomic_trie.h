#ifndef _ATOMIC_TRIE_H_
#define _ATOMIC_TRIE_H_

#include <stdlib.h>
#include <pthread.h>

#include "list.h"
#include "bitmask.h"

#define ALPHABET_SIZE 26
#define BITMASK_SIZE 1024

#define FILE_HAS_WORD '1'


typedef struct Node {
    void *value;
    pthread_mutex_t lock;
    struct Node **children;
    char letter;
} Node;


typedef struct AtomicTrie {
    Node *root;
} AtomicTrie;


typedef struct KeyValuePair {
    char *key;
    void *value;
} KeyValuePair;


AtomicTrie *create_atomic_tree();
int has_word_from_file(AtomicTrie *trie, char *word, int file_id);
void insert_word_from_file(AtomicTrie *trie, char *word, int file_id);
void add_word_in_file_group(AtomicTrie *trie, char *word, int file_id);
void get_words_starting_with_letter(AtomicTrie *trie, char letter, List *words);
void destroy_atomic_trie(AtomicTrie **trie);


#endif
