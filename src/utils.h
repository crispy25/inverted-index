#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>

#define CHECK_MALLOC(x) if (x == NULL) {    \
    perror("Couldn't allocate memory");     \
    exit(-1);                               \
}

#define CHECK_FILE(f) if (f == NULL) {      \
    perror("Couldn't open file");           \
    exit(-1);                               \
}


#define SAFE_FREE(x) if (x) free(x)


#define MIN(x, y) (x > y ? y : x)

#endif
