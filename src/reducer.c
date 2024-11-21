#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "utils.h"
#include "reducer.h"
#include "structs.h"
#include "atomic_trie.h"


// Open a file and write all words with the files they appear in
void write_result(char letter, List *words)
{
    char file_name[] = "a.txt";
    file_name[0] = letter;
    FILE *file = fopen(file_name, "w");
    CHECK_FILE(file);

    for (ListNode *node = words->head; node; node = node->next) {
        KeyValuePair *word_info = node->data;
        fprintf(file, "%s:[", word_info->key);

        int is_empty = 1;
        for (int i = START_ID; i < NODE_BITMASK_SIZE; i++)
            if (has_bit(word_info->value, i)) {
                if (is_empty) {
                    fprintf(file, "%d", i);
                    is_empty = 0;
                }
                else {
                    fprintf(file, " %d", i);
                }
            }

        fprintf(file, "]\n");
    }

    fclose(file);
}


// Compare word frequency in distinct files, and if equal, lexicographically
int compare_word_freq(const void *x, const void *y)
{
    const KeyValuePair *kv1 = ((ListNode*)x)->data, *kv2 = ((ListNode*)y)->data;
    return compare_bitmasks(kv2->value, kv1->value);
}


// Reducer thread task
void *reducer(void *arg)
{
    ThreadArgs *args = (ThreadArgs*)arg;

    // Wait for Mapper threads to finish
    pthread_barrier_wait(&args->job->barrier);
    
    int start, end;

    start = args->id * (double)ALPHABET_SIZE / args->job->args->R;
    end = (args->id + 1) * (double)ALPHABET_SIZE / args->job->args->R;
    end = MIN(end, ALPHABET_SIZE);

    for (int i = start; i < end; i++)
        get_words_starting_with_letter(args->trie, 'a' + i, args->results[i]);

    for (int i = start; i < end; i++)
        sort_list(args->results[i], compare_word_freq);

    for (int i = start; i < end; i++)
        write_result('a' + i, args->results[i]);

    return NULL;
}
