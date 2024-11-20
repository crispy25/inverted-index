#ifndef _BITMASK_H_
#define _BITMASK_H_

#define NODE_BITMASK_SIZE 128 * 8
#define NODE_BITMASK_BLOCK_SIZE 8
#define NODE_BITMASK_ALLOC_SIZE 128, 1

typedef struct Bitmask {
    int ones;
    char *bits;
} Bitmask;

Bitmask *create_bitmask();

int has_bit(Bitmask *bitmask, int pos);

void set_bit(Bitmask *bitmask, int pos);

void clear_bit(Bitmask *bitmask, int pos);

int compare_bitmasks(Bitmask *bitmask1, Bitmask *bitmask2);

#endif
