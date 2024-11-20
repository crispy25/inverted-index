#include <stdlib.h>

#include "bitmask.h"
#include "utils.h"

Bitmask *create_bitmask()
{
    Bitmask *bitmask = calloc(1, sizeof(*bitmask));
    CHECK_MALLOC(bitmask);

    bitmask->bits = calloc(NODE_BITMASK_ALLOC_SIZE);
    CHECK_MALLOC(bitmask->bits);

    return bitmask;
}


int has_bit(Bitmask *bitmask, int pos)
{
    return (bitmask->bits[pos / NODE_BITMASK_BLOCK_SIZE] & (1 << (pos % NODE_BITMASK_BLOCK_SIZE)));
}

void set_bit(Bitmask *bitmask, int pos)
{
    bitmask->ones += !has_bit(bitmask, pos);
    bitmask->bits[pos / NODE_BITMASK_BLOCK_SIZE] |= (1 << (pos % NODE_BITMASK_BLOCK_SIZE));
}

void clear_bit(Bitmask *bitmask, int pos)
{
    bitmask->bits[pos / NODE_BITMASK_BLOCK_SIZE] &= ~(1 << (pos % NODE_BITMASK_BLOCK_SIZE));
}

int compare_bitmasks(Bitmask *bitmask1, Bitmask *bitmask2)
{
    return bitmask1->ones > bitmask2->ones;
}