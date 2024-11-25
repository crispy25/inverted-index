#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

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


// Get the frequency of a word stored in a node
int get_word_freq(void *node)
{
    return ((Bitmask*)((KeyValuePair*)((ListNode*)node)->data)->value)->ones;
}


// Pick the next available letter to process
int pick_a_letter(JobInfo *job)
{
    int index = -1;

    pthread_mutex_lock(&job->task_lock);
    if (job->result_index < ALPHABET_SIZE) {
        index = job->result_index;
        job->result_index++;
    }
    pthread_mutex_unlock(&job->task_lock);

    return index;
}


// Reducer thread task
void *reducer(void *arg)
{
    ThreadArgs *args = (ThreadArgs*)arg;

    // Wait for Mapper threads to finish
    pthread_barrier_wait(&args->job->barrier);

    int start = args->id * (double)ALPHABET_SIZE / args->job->args->R;
    int end = (args->id + 1) * (double)ALPHABET_SIZE / args->job->args->R;
    end = MIN(end, ALPHABET_SIZE);

    for (int index = start; index < end; index++)
        get_words_starting_with_letter(args->trie, 'a' + index, args->results[index]);

    pthread_barrier_wait(&args->job->reducers_barrier);

    int index;
    while ((index = pick_a_letter(args->job)) >= 0) {
        sort_list(args->results[index], get_word_freq);
        write_result('a' + index, args->results[index]);
    }

    return NULL;
}
