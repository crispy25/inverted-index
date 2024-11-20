#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "list.h"
#include "utils.h"
#include "bitmask.h"
#include "atomic_trie.h"

#define MAX_FILE_NAME_LEN 128
#define FILE_AVAILABLE 0
#define FILE_TAKEN 1
#define START_ID 0

#define IS_LETTER(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))


typedef struct Args {
    int M, R;
    char file[MAX_FILE_NAME_LEN];
} Args;

typedef struct FileInfo {
    char *name;
    long size;
    int id;
} FileInfo;

typedef struct JobInfo {
    Args *args;
    FileInfo *files_info;
    pthread_mutex_t task_lock;
    size_t file_count, file_index;
    pthread_barrier_t barrier, reducers_barrier;
} JobInfo;

typedef struct WordInfo {
    char *word;
    int file_id;
} WordInfo;

typedef struct ThreadArgs {
    int id;
    AtomicTrie *trie;
    JobInfo *job;
    List **final_results;
    WordInfo *aggregate_words[ALPHABET_SIZE];
    List **partial_results;
} ThreadArgs;


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


JobInfo *get_job_info(Args *args)
{
    JobInfo *job_info = calloc(1, sizeof(*job_info));
    CHECK_MALLOC(job_info);

    job_info->args = args;

    pthread_mutex_init(&job_info->task_lock, NULL);
    pthread_barrier_init(&job_info->barrier, NULL, args->M + args->R);
    pthread_barrier_init(&job_info->reducers_barrier, NULL, args->R);

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

        // puts(job_info->files_info[index].name);
        FILE *file = fopen(job_info->files_info[index].name, "rb");
        CHECK_FILE(file);

        // Get file size
        fseek(file, 0, SEEK_END);
        long r = ftell(file);
        // printf("%s %ld\n", job_info->files_info[index].name, r);
        job_info->files_info[index].size = r;
        fseek(file, 0, SEEK_SET);

        job_info->files_info[index].id = index + 1;
        fclose(file);

        index++;
        file_count--;
    }
    puts("Done reading files");
    fclose(master_file);

    return job_info;
}


void free_job_info(JobInfo **job_info)
{
    for (int i = 0; i < (*job_info)->file_count; i++) {
        free((*job_info)->files_info[i].name);
    }

    pthread_mutex_destroy(&(*job_info)->task_lock);
    pthread_barrier_destroy(&(*job_info)->barrier);   
    pthread_barrier_destroy(&(*job_info)->reducers_barrier);   

    free((*job_info)->files_info);
    free(*job_info);
}


void free_word_info(void *data) {
    WordInfo *word_info = ((ListNode*)data)->data;
    free(word_info->word);
    free(word_info);
    free(data);
}


void free_word(void *data)
{
    free(((ListNode*)data)->data);
    free(data);
}


int pick_file(JobInfo *job)
{
    if (job->file_index >= job->file_count)
        return -1;

    int file_id = -1;
    pthread_mutex_lock(&job->task_lock);
    file_id = job->file_index;
    job->file_index += 1;
    pthread_mutex_unlock(&job->task_lock);
    return file_id;
}


char *format_word(char *word)
{
    size_t len = strlen(word), idx = 0;
    char *new_word = calloc(len + 1, sizeof(char));
    CHECK_MALLOC(new_word);

    for (int i = 0; i < len; i++) {
        if (word[i] >= 'a' && word[i] <= 'z')
            new_word[idx++] = word[i];
        else if (word[i] >= 'A' && word[i] <= 'Z')
            new_word[idx++] = word[i] + 32;
    }

    new_word[idx] = 0;

    return new_word;
}


char *get_next_word(char *str, size_t size, int *offset) {
    int index = *offset, len = 0;
    char buffer[128] = {0};

    while (index < size && !IS_LETTER(str[index]))
        index++;
    
    while (index < size) {
        if (str[index] == ' ' || str[index] == '\n') {
            index++;
            break;
        }
        
        if (!IS_LETTER(str[index])) {
            index++;
            continue;;
        }

        buffer[len] = str[index] + 32 * (str[index] >= 'A' && str[index] <= 'Z');
   
        index++;
        len++;
    }

    *offset = index;
    if (len == 0)
        return NULL;

    char *word = calloc(len + 1, sizeof(char));
    CHECK_MALLOC(word);
    memcpy(word, buffer, len);

    return word;
}


void process_file(FileInfo *file_info, AtomicTrie *trie, List *words_info)
{
    int fd = open(file_info->name, O_RDONLY, S_IRUSR | S_IWUSR);

    char *file_map = mmap(NULL, file_info->size, PROT_READ, MAP_PRIVATE, fd, 0);
    CHECK_MALLOC(file_map);

    int offset = 0;
    char *word = get_next_word(file_map, file_info->size, &offset);

    while (word) {

        if (!has_word_from_file(trie, word, file_info->id)) {
            WordInfo *word_info = calloc(1, sizeof(*word_info));
            CHECK_MALLOC(word_info);

            word_info->word = word;
            word_info->file_id = file_info->id;

            insert_word_from_file(trie, word, file_info->id);
            push_back(words_info, word_info);
            
        } else {
            free(word);
        }
        word = get_next_word(file_map, file_info->size, &offset);
    }

    munmap(file_map, file_info->size);

    close(fd);
}


void *mapper(void *arg)
{
    ThreadArgs *args = (ThreadArgs*)arg;
    int file_index;

    while ((file_index = pick_file(args->job)) >= 0) {
        printf("Proccesing file %d\n", file_index);
        process_file(&args->job->files_info[file_index], args->trie, args->partial_results[args->id]);
    }

    puts("Done mapping");
    pthread_barrier_wait(&args->job->barrier);
    return NULL;
}


void write_result(char letter, List *words)
{
    char file_name[5] = "a.txt";
    file_name[0] = letter;
    FILE *file = fopen(file_name, "w");
    CHECK_FILE(file);

    for (ListNode *node = words->head; node; node = node->next) {
        KeyValuePair *word_info = node->data;
        fprintf(file, "%s:[", word_info->key);
        // List *files = (List*)word_info->value;
        // for (ListNode *fnode = files->head; fnode; fnode = fnode->next) {
        //     fprintf(file, "%d", *(int*)fnode->data);
            
        //     if (fnode->next)
        //         fprintf(file, " ");
        // }
        // puts(word_info->value);

        int is_empty = 1;
        for (int i = START_ID; i < BITMASK_SIZE; i++)
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

    for (ListNode *node = words->head; node; node = node->next) {
        KeyValuePair *word_info = node->data;
        // List *files = (List*)word_info->value;
        // for (ListNode *fnode = files->head; fnode; fnode = fnode->next) {
        //     free(fnode->data);
        // }
        // free_nodes(files, free);
        free(((Bitmask *)word_info->value)->bits);
        // free(word_info->value);


        free(word_info->key);
        free(word_info);
    }

    free_list(&words, free);

    fclose(file);
}


int compare_word_freq(const void *x, const void *y)
{
    const KeyValuePair *kv1 = ((ListNode*)x)->data, *kv2 = ((ListNode*)y)->data;
    // unsigned long long freq1 = *((unsigned long long*)kv1->value); //((List*)kv1->value)->size;
    // unsigned long long freq2 = *((unsigned long long*)kv2->value);//((List*)kv2->value)->size;
    int r = compare_bitmasks(kv2->value, kv1->value);
    return r;
}


void *reducer(void *arg)
{
    ThreadArgs *args = (ThreadArgs*)arg;

    pthread_barrier_wait(&args->job->barrier);

    int start = args->id * (double)args->job->args->M / args->job->args->R;
    int end = (args->id + 1) * (double)args->job->args->M / args->job->args->R;
    end = MIN(end, args->job->args->M);

    // for (int i = start; i < end; i++)
    //     for (ListNode *node = args->partial_results[i]->head; node; node = node->next)
    //         insert_word_from_file(args->trie, ((WordInfo*)node->data)->word, ((WordInfo*)node->data)->file_id);

    // pthread_barrier_wait(&args->job->reducers_barrier);

    start = args->id * (double)ALPHABET_SIZE / args->job->args->R;
    end = (args->id + 1) * (double)ALPHABET_SIZE / args->job->args->R;
    end = MIN(end, ALPHABET_SIZE);

    for (int i = start; i < end; i++)
        get_words_starting_with_letter(args->trie, 'a' + i, args->final_results[i]);

    puts("Sorting...");
    for (int i = start; i < end; i++) {
        // printf("List %d done\n", i);
        sort_list(args->final_results[i], compare_word_freq);
    }

    for (int i = start; i < end; i++)
        write_result('a' + i, args->final_results[i]);

    // printf("Reducer %d done!\n", args->id);
    return NULL;
}


void run_job(JobInfo *job)
{
    size_t thread_count = job->args->M + job->args->R;

    pthread_t threads[thread_count];
    void *status;
    int r;

    List **partial_results = calloc(job->args->M, sizeof(List *));
    CHECK_MALLOC(partial_results);

    for (int i = 0; i < job->args->M; i++)
        partial_results[i] = create_list();

    List *final_results[ALPHABET_SIZE];
    for (int i = 0; i < ALPHABET_SIZE; i++)
        final_results[i] = create_list();

    AtomicTrie *mappersTrie = create_atomic_tree();
    AtomicTrie *reducersTrie = create_atomic_tree();

    ThreadArgs thread_args[thread_count];

    // Arguments for threads
    for (int i = 0; i < thread_count; i++) {
        thread_args[i].job = job;
        thread_args[i].id = i - (job->args->M * (i >= job->args->M));
        thread_args[i].partial_results = partial_results;
        thread_args[i].final_results = final_results;
        thread_args[i].trie = mappersTrie; // i < job->args->M? mappersTrie : reducersTrie;
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

    for (int i = 0; i < job->args->M; i++)
        free_list(&partial_results[i], free_word_info);
    free(partial_results);

    destroy_atomic_trie(&mappersTrie);
    destroy_atomic_trie(&reducersTrie);
}


int main(int argc, char **argv)
{
    Args args = parse_args(argc, argv);
    JobInfo *job = get_job_info(&args);

    run_job(job);

    free_job_info(&job);
    return 0;
}