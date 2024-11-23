#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"
#include "mapper.h"
#include "structs.h"
#include "atomic_trie.h"

// Pick the next available file
int pick_file(JobInfo *job)
{
    int file_id = -1;

    pthread_mutex_lock(&job->task_lock);    
    if (job->file_index < job->file_count) {
        file_id = job->file_index;
        job->file_index += 1;
    }
    pthread_mutex_unlock(&job->task_lock);

    return file_id;
}


// Get next space separated word in str from the given position
// Returns the word length
int get_next_word(char *str, size_t size, int *offset, char *word) {
    int index = *offset, len = 0;
    memset(word, 0, MAX_WORD_LEN);

    while (index < size && !IS_LETTER(str[index]))
        index++;
    
    while (index < size && len < MAX_WORD_LEN) {
        if (str[index] == ' ' || str[index] == '\n') {
            index++;
            break;
        }
        
        if (!IS_LETTER(str[index])) {
            index++;
            continue;;
        }

        word[len] = str[index] + 32 * (str[index] >= 'A' && str[index] <= 'Z');
   
        index++;
        len++;
    }

    *offset = index;
    return len;
}


// Open and read the file into memory and add each word in the file in the trie
// with the corresponding file id
void process_file(FileInfo *file_info, AtomicTrie *trie)
{
    int fd = open(file_info->name, O_RDONLY, S_IRUSR | S_IWUSR);

    char *file_map = mmap(NULL, file_info->size, PROT_READ, MAP_PRIVATE, fd, 0);
    CHECK_MALLOC(file_map);

    int offset = 0;
    char word[MAX_WORD_LEN];

    while (get_next_word(file_map, file_info->size, &offset, word)) {
        if (!has_word_from_file(trie, word, file_info->id))
            insert_word_from_file(trie, word, file_info->id);
    }

    munmap(file_map, file_info->size);

    close(fd);
}


// Mapper thread task
void *mapper(void *arg)
{
    ThreadArgs *args = (ThreadArgs*)arg;

    int file_index;
    while ((file_index = pick_file(args->job)) >= 0) {
        process_file(&args->job->files_info[file_index], args->trie);
    }

    // Notify reducer threads that one mapper thread finish
    pthread_barrier_wait(&args->job->barrier);
    return NULL;
}
