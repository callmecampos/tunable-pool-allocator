/**
 * Scratchpad main file.
 * 
 * Written by Felipe Campos, 11/12/2020.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "pool_alloc.h"

int main(int argc, char *argv[])
{
    const size_t arr[] = {4096};
    bool pool = pool_init(arr, 1);

    if (!pool) {
        printf("sad.");
        return 0;
    }

    for (int i = 0; i < 20; i++) {
        printf("%p\n", pool_alloc(4096));
    }

    /* uint8_t *last_ptr = pool_alloc(4096);
    ck_assert(pool_alloc(4096) == NULL);

    pool_free(last_ptr);

    ck_assert(pool_alloc(4096) != NULL); */

    return 0;
}