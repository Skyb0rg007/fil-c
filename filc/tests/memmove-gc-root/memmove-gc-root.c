// memmove_aux_loop_body holds `lower` untracked across the exit in ensure_aux.
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdfil.h>

#define BIG (128u<<20)
#define NSPRAY 20000

__attribute__((noinline)) static void setup(void **table)
{
    char *p = malloc(64);
    memset(p, 'P', 64);
    table[0] = p;
    free(p);
}

__attribute__((noinline)) static void clobber_stack(void)
{
    volatile char buf[4096];
    memset((void*)buf, 0, sizeof buf);
}

static char *spray[NSPRAY];

int main(int argc, char **argv)
{
    int iters = argc > 1 ? atoi(argv[1]) : 200;
    for (int it = 0; it < iters; it++) {
        void **table = malloc(16 * sizeof(void*));
        void **big = malloc(BIG);
        setup(table);
        clobber_stack();
        memcpy(big, table, 16 * sizeof(void*));   // <- window
        char *d = big[0];
        for (int i = 0; i < NSPRAY; i++) {
            spray[i] = malloc(64);
            memset(spray[i], 'Q', 64);
            ZASSERT(zgetlower(spray[i]) != zgetlower(d));
        }
        memset(spray, 0, sizeof spray);
        free(big);
        zgc_request_and_wait();
    }
    puts("done");
    return 0;
}
