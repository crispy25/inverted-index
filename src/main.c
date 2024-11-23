#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "list.h"
#include "utils.h"
#include "mapper.h"
#include "reducer.h"
#include "bitmask.h"
#include "structs.h"
#include "atomic_trie.h"


// Get program arguments
Args parse_args(int argc, char **argv)
{
    if (argc != 4) {
        printf("To run: ./tema1 M R file\n");
        exit(0);
    }

    int M = atoi(argv[1]);
    int R = atoi(argv[2]);

    if (M <= 0 || R <= 0) {
        printf("Thread count must be greater than 0!\n");
        exit(0);
    }

    Args args = {.M = M, .R = R};
    strcpy(args.file, argv[3]);
    return args;
}


// Create and return a new job with the given arguments
JobInfo *init_job(Args *args)
{
    JobInfo *job_info = calloc(1, sizeof(*job_info));
    CHECK_MALLOC(job_info);

    job_info->args = args;

    pthread_mutex_init(&job_info->task_lock, NULL);
    pthread_barrier_init(&job_info->barrier, NULL, args->M + args->R);

    FILE* master_file = fopen(args->file, "r");
    if (!master_file) {
        printf("Couldn't open master file!\n");
        exit(0);
    }

    int file_count;
    fscanf(master_file, "%d\n", &file_count);
    job_info->file_count = file_count;
    job_info->files_info = calloc(file_count, sizeof(*job_info->files_info));
    CHECK_MALLOC(job_info->files_info);

    for (int i = 0; i < file_count; i++) {
        job_info->files_info[i].name = calloc(MAX_FILE_NAME_LEN, sizeof(char));
        CHECK_MALLOC(job_info->files_info[i].name);
    }

    int index = 0;
    while (file_count > 0) {
        // Read and open file
        fgets(job_info->files_info[index].name, MAX_FILE_NAME_LEN, master_file);
        if (file_count > 1)
            job_info->files_info[index].name[strlen(job_info->files_info[index].name) - 1] = 0;

        FILE *file = fopen(job_info->files_info[index].name, "rb");
        CHECK_FILE(file);

        // Get file size
        fseek(file, 0, SEEK_END);
        job_info->files_info[index].size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Set file id
        job_info->files_info[index].id = index + 1;
        fclose(file);

        index++;
        file_count--;
    }

    fclose(master_file);

    return job_info;
}


// Run the given job
void run_job(JobInfo *job)
{
    size_t thread_count = job->args->M + job->args->R;

    pthread_t threads[thread_count];
    void *status;
    int r;

    List *results[ALPHABET_SIZE];
    for (int i = 0; i < ALPHABET_SIZE; i++)
        results[i] = create_list();

    AtomicTrie *trie = create_atomic_tree();

    ThreadArgs thread_args[thread_count];

    // Arguments for threads
    for (int i = 0; i < thread_count; i++) {
        thread_args[i].job = job;
        thread_args[i].id = i - (job->args->M * (i >= job->args->M));
        thread_args[i].results = results;
        thread_args[i].trie = trie;
    }

    // Create threads
    for (int i = 0; i < thread_count; i++) {
        void* (*task)(void*) = i < job->args->M? mapper : reducer;

        r = pthread_create(&threads[i], NULL, task, &thread_args[i]);

        if (r) {
            perror("Couldn't create thread: ");
            exit(-1);
        }
    }

    // Wait for threads
    for (int i = 0; i < thread_count; i++) {
        r = pthread_join(threads[i], &status);

        if (r) {
            perror("Error while wait thread: ");
            exit(-1);
        }
    }

    // Free results
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        for (ListNode *node = results[i]->head; node; node = node->next) {
            KeyValuePair *word_info = node->data;
            free(((Bitmask *)word_info->value)->bits);
            free(word_info->key);
            free(word_info);
        }
        free_list(&results[i], free);
    }
    
    destroy_atomic_trie(&trie);
}


// Free resources used by job
void free_job_info(JobInfo **job_info)
{
    for (int i = 0; i < (*job_info)->file_count; i++) {
        free((*job_info)->files_info[i].name);
    }

    pthread_mutex_destroy(&(*job_info)->task_lock);
    pthread_barrier_destroy(&(*job_info)->barrier);   

    free((*job_info)->files_info);
    free(*job_info);
}


int main(int argc, char **argv)
{
    Args args = parse_args(argc, argv);
    JobInfo *job = init_job(&args);

    run_job(job);

    free_job_info(&job);
    return 0;
}
