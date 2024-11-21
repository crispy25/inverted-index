#ifndef _ATOMIC_TRIE_H_
#define _ATOMIC_TRIE_H_

#include <stdlib.h>
#include <pthread.h>

#include "list.h"
#include "bitmask.h"

#define ALPHABET_SIZE 26


typedef struct TrieNode {
    void *value;
    pthread_mutex_t lock;
    struct TrieNode **children;
    char letter;
} TrieNode;


typedef struct AtomicTrie {
    TrieNode *root;
} AtomicTrie;


AtomicTrie *create_atomic_tree();
int has_word_from_file(AtomicTrie *trie, char *word, int file_id);
void insert_word_from_file(AtomicTrie *trie, char *word, int file_id);
void get_words_starting_with_letter(AtomicTrie *trie, char letter, List *words);
void destroy_atomic_trie(AtomicTrie **trie);

#endif
