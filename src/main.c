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
    const size_t arr[] = {sizeof(int), 1024, 2048};
    printf("I'm on a skitz mission (%p) (%zu)\n", arr, sizeof(int));
    bool pool = pool_init(arr, 3);
    if (pool)
    {
        int *p1 = pool_alloc(sizeof(int));
        assert(p1 != NULL);
        *p1 = 4;
        printf("Allocated (%p) (%d)\n", p1, *p1);
        int *p2 = pool_alloc(sizeof(int));
        assert(p2 != NULL);
        *p2 = 69;
        printf("We fackin did it (%p) (%d)\n", p2, *p2);
        pool_free(p1);
        pool_free(p2);

        int *p3 = pool_alloc(sizeof(int));
        assert(p3 != NULL);
        *p3 = 420;
        printf("Allocated (%p) (%d)\n", p3, *p3);
        int *p4 = pool_alloc(sizeof(int));
        assert(p4 != NULL);
        *p4 = 100;
        printf("We fackin did it (%p) (%d)\n", p4, *p4);
        pool_free(p3);
        pool_free(p4);
    }
    return 0;
}