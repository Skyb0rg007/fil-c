// memmove's store barrier (fugc_mark) does leaf-marking; races with aux installation by a
// thread that has not yet seen filc_current_marking_state != 0.  Thread T1 is "preempted" (by gdb)
// between reading the marking state and CASing the aux in ensure_aux_ptr_slow.
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdfil.h>

static void **X;
static volatile int ready, done;
static volatile int marker_count;
__attribute__((noinline)) void marker(void) { marker_count++; }

static void *t1(void *arg)
{
    char *p = malloc(32);
    strcpy(p, "hello from p");
    ready = 1;
    marker();
    X[0] = p;      /* first pointer store into X: allocates + installs the aux */
    done = 1;
    return NULL;
}

static double now(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }

#define NSPRAY 4000
static uintptr_t *spray[NSPRAY];

int main(void)
{
    X = malloc(64);
    void **src = malloc(128), **dst = malloc(128);
    src[0] = X;
    dst[1] = X; dst[1] = NULL;      /* make sure dst has an aux */
    zgc_request_and_wait();
    pthread_t t;
    pthread_create(&t, NULL, t1, NULL);
    while (!ready) ;
    usleep(300000);                 /* T1 is now parked in the window */
    zgc_request_fresh();            /* GC: marking_state = marking; handshake blocks on T1 */
    double end = now() + 1.0;
    while (now() < end)
        memmove(dst, src, 128);     /* barrier: fugc_mark(X) -> "leaf" */
    while (!done) ;
    pthread_join(t, NULL);
    zprintf("X[0] before GC finishes: %P\n", X[0]);
    zgc_request_and_wait();
    for (int i = 0; i < NSPRAY; i++) {
        spray[i] = malloc(48);      /* 16 + 48 = 64 = size class of X's aux */
        for (int j = 0; j < 6; j++) spray[i][j] = 0x4141414141414140ul;
    }
    zprintf("X[0] after GC + spray:   %P\n", X[0]);
    zprintf("X[3] after GC + spray:   %P\n", X[3]);
    return 0;
}
