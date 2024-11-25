#ifndef _STRUCTS_H_
#define _STRUCTS_H_

#include "list.h"
#include "utils.h"
#include "atomic_trie.h"

typedef struct Args {
    int M, R;
    char file[MAX_FILE_NAME_LEN];
} Args;


typedef struct FileInfo {
    char *name;
    long size;
    int id;
} FileInfo;


typedef struct KeyValuePair {
    char *key;
    void *value;
} KeyValuePair;


typedef struct JobInfo {
    Args *args;
    FileInfo *files_info;
    pthread_mutex_t task_lock;
    size_t file_count, file_index, result_index;
    pthread_barrier_t barrier, reducers_barrier;
} JobInfo;


typedef struct ThreadArgs {
    int id;
    AtomicTrie *trie;
    JobInfo *job;
    List **results;
} ThreadArgs;


#endif
