#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../src/pool_alloc.h"

int main(int argc, char* argv[]) {
    const size_t arr[] = {sizeof(int), 1024, 2048};
    printf("I'm on a skitz mission (%p)\n", arr);
    bool pool = pool_init(arr, 3);
    if (pool) {
        // int y = 4;
        // int* x = &y;
        int* x = pool_alloc(sizeof(int));
        if (x == NULL) {
            printf("Fucked.\n");
            return 0;
        }
        *x = 4;
        printf("Allocated (%p) (%d)\n", x, *x);
        pool_free(x);
        printf("We fackin did it (%p) (%d)\n", x, pool);
    }
    return 0;
}
