#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../src/pool_alloc.h"

int main(int argc, char* argv[]) {
    const size_t arr[] = {sizeof(int), 1024, 2048};
    printf("I'm on a skitz mission (%p)", arr);
    bool pool = pool_init(arr, 3);
    if (pool) {
        int* x = pool_alloc(sizeof(int));
        *x = 4;
        printf("Allocated (%p) (%d)", x, *x);
        pool_free(x);
        printf("We fackin did it (%p) (%d)\n", x, pool);
    }
    return 0;
}
