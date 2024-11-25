## Homework 1 APD - Inverted Index using Map-Reduce paradigm

### Project description
- The project consists of a parallel implementation of finding the inverted
index for a list of files given as input.


### Project structure

+ structs.h, utils.h -> contains typedefs, structs and useful macros

+ atomic_trie.h, atomic_trie.c -> implementation of an atomic trie,
with a mutex lock on each node. Each terminal node has as value a bitmask.

+ list.h, list.c -> implementation of linked list

+ bitmask.h, bitmask.c -> implementation of a bitmask with size of 1024

+ mapper.h, mapper.c -> mapper thread task implementation

+ reducer.h, reducer.c -> reducer thread task implementation

+ main.c -> contains initialization, following start and execution of threads


### Implementation
+ The program parses the arguments passed in the CLI and allocates a new job
to handle the request.
+ In the job initialization, the files that need to be processed are read from
the file given as arument. For each file, the name, size and id is
stored in an array.
+ When the job starts running, it allocates and initialize resources for the
thread arguments, shared variables between threads and synchronization
variables: mutex and barriers. It follows the creation of threads and waiting
for them to finish.
+ Each thread has as arguments the job that started the thread, the thread id,
a shared atomic trie and list of results. 
+ The mappers threads start taking one file to process in a dynamic manner, as
long there are any files left. When a file is taken by a mapper thread, it is
brought in memory using *mmap* and then each space separated word is inserted
in the atomic trie. The node storing the last letter updates the node bitmask,
setting the bit on the position given by the file's id to one.
+ When a mapper thread finish a file and there are no more files to be
processed, it waits for the rest of the mapper threads to finish.
+ All reducer threads wait for all the mapper thread to finish, using a barrier
with the value set as number of total threads.
+ The reducer threads are given a equal amount of letters from the alphabet.
For each letter, the thread gets the words starting with the letter from
the atomic trie and groupes them in a list. After the sub-task is done, the
thread waits for the other reducer threads.
+ After all the reducer threads finish the first step, each reducer thread
picks a letter, sorts the list containing the words starting with the letter
and finally writes the result in a new file named after the letter. The words
are already alphabetically sorted, meaning the words are only sorted after the
number of ones in the bitmask. Each word is printed on a new line, followed by
the file ids they appear in.
+ After all threads joined the main thread, the used resources are freed.


### Observations
+ No memory leaks.
+ When the file is parsed, the words are formatted to lowercase before
inserted in the trie.
+ The words are taken from the trie in a alphabetically order, using
preorder traversal.
+ The push_back and push_front of the linked list have O(1) time complexity.
+ The bitmask uses 128 bytes for the actual bitmask and 4 bytes to count the
number the ones in it. The bitmask is accessed at the bit level.
+ The checker gives 84/84 after multiple runs.


## Copyright Popescu Cristian-Emanuel 332CA
