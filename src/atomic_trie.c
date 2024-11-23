#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "structs.h"
#include "atomic_trie.h"


AtomicTrie *create_atomic_tree()
{
    AtomicTrie *trie = malloc(sizeof(*trie));
    CHECK_MALLOC(trie);

    trie->root = calloc(1, sizeof(*trie->root));

    CHECK_MALLOC(trie->root);
    pthread_mutex_init(&trie->root->lock, NULL);

    return trie;
}


int _has_word(TrieNode *node, char *word, int len, int file_id)
{
    if (len == 0) {
        if (node->value == NULL)
            return 0;
        return has_bit(node->value, file_id);
    }
    
    if (node->children == NULL) {
        return 0;
    }

    if (node->children[word[0] - 'a'] == NULL) {
        return 0;
    }
    
    return _has_word(node->children[word[0] - 'a'], word + 1, len - 1, file_id);
}


int has_word_from_file(AtomicTrie *trie, char *word, int file_id)
{
    if (!word)
        return 1;

   return _has_word(trie->root, word, strlen(word), file_id); 
}


void _insert_word_from_file(TrieNode *node, char *word, int len, int file_id)
{
    if (len == 0) {
        pthread_mutex_lock(&node->lock);
        if (node->value == NULL) {
            node->value = create_bitmask();
            CHECK_MALLOC(node->value);
        }
        set_bit(node->value, file_id);
        pthread_mutex_unlock(&node->lock);

        return;
    }

    pthread_mutex_lock(&node->lock);
    if (node->children == NULL) {
        node->children = calloc(ALPHABET_SIZE, sizeof(TrieNode*));
        CHECK_MALLOC(node->children);
    }
    pthread_mutex_unlock(&node->lock);


    int key = word[0] - 'a';
    pthread_mutex_lock(&node->lock);
    if (node->children[key] == NULL) {
        node->children[key] = calloc(1, sizeof(TrieNode));
        CHECK_MALLOC(node->children[key]);
    
        node->children[key]->letter = word[0];
        pthread_mutex_init(&node->children[key]->lock, NULL);
    
        pthread_mutex_unlock(&node->lock);

        _insert_word_from_file(node->children[key], word + 1, len - 1, file_id);
        return;
    }
    pthread_mutex_unlock(&node->lock);

    _insert_word_from_file(node->children[key], word + 1, len - 1, file_id);
}


void insert_word_from_file(AtomicTrie *trie, char *word, int file_id)
{
    if (!word)
        return;

    _insert_word_from_file(trie->root, word, strlen(word), file_id);
}


void _get_words_starting_with_letter(TrieNode *node, char *word, int len, List *words)
{
    // Node represens last letter in a word
    if (node->value) {
        word[len] = 0;
        KeyValuePair *word_info = malloc(sizeof(*word_info));
        CHECK_MALLOC(word_info);

        word_info->key = calloc(len + 1, sizeof(char));
        CHECK_MALLOC(word_info->key);

        memcpy(word_info->key, word, len + 1);
        word_info->value = node->value;

        push_back(words, word_info);
    }

    if (node->children == NULL)
        return;

    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (node->children[i]) {
            word[len] = 'a' + i;
            _get_words_starting_with_letter(node->children[i], word, len + 1, words);
        }
    }
}


void get_words_starting_with_letter(AtomicTrie *trie, char letter, List *words)
{
    if (trie->root->children == NULL)
        return;

    if (trie->root->children[letter - 'a'] == NULL)
        return;
    
    char word[MAX_WORD_LEN] = {0};
    word[0] = letter;

    _get_words_starting_with_letter(trie->root->children[letter - 'a'], word, 1 , words);
}


void _destroy_node(TrieNode *node)
{
    if (node == NULL)
        return;

    if (node->children) {
        for (int i = 0; i < ALPHABET_SIZE; i++)
            _destroy_node(node->children[i]);
        free(node->children);
    }

    if (node->value)
        free(node->value);

    pthread_mutex_destroy(&node->lock);
    free(node);
}


void destroy_atomic_trie(AtomicTrie **trie)
{
    _destroy_node((*trie)->root);
    free(*trie);
}
