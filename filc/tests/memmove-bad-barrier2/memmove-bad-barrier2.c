// Race: T1 lazily creates the aux of X right as marking turns on (aux allocated white, T1 saw !marking)
//       T2 memmove()s a pointer to X into a heap array that already has an aux, seeing marking on.
// memmove's barrier uses fugc_mark() *with* the leaf optimisation -> X marked black, never scanned,
// X's aux never marked -> aux swept while X is live.
// The program is data-race free at the C level: T1 writes X->p, T2 only copies pointers-to-X and X->c is atomic.
#include <stdfil.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct obj { long* p; _Atomic long c; } obj;
#define N 40000
#define K 6
typedef struct ring { obj* slot[N]; } ring;
static ring* _Atomic cur;
static ring* old[K];
static _Atomic long latest;
static volatile int stop;
static long* sentinel;
static volatile size_t eight = 8;

static void* t2(void* arg)
{
    obj** dst = malloc(sizeof(obj*) * N);
    dst[0] = (obj*)sentinel; // force aux
    while (!stop) {
        ring* r = atomic_load(&cur);
        long i = atomic_load(&latest);
        if (!r || i < 0) continue;
        memmove(&dst[i], &r->slot[i], eight);
        obj* x = dst[i];
        if (x) atomic_fetch_add_explicit(&x->c, 1, memory_order_relaxed); // keep the line bouncing
    }
    return dst;
}

static void validate(ring* r)
{
    if (!r) return;
    for (long i = 0; i < N; ++i) {
        obj* x = r->slot[i];
        if (!x) continue;
        if (*x->p != 0x1234567890abcdefl) { zprintf("BAD value via %P\n", x->p); abort(); }
    }
}

int main(int argc, char** argv)
{
    int secs = argc > 1 ? atoi(argv[1]) : 30;
    sentinel = malloc(sizeof(long)); *sentinel = 0x1234567890abcdefl;
    pthread_t th[2];
    int nt2 = argc > 2 ? atoi(argv[2]) : 1;
    atomic_store(&latest, -1);
    for (int i = 0; i < nt2; ++i) pthread_create(&th[i], 0, t2, 0);
    time_t end = time(0) + secs;
    unsigned long rounds = 0;
    while (time(0) < end) {
        ring* r = calloc(1, sizeof(ring));
        r->slot[0] = 0;
        atomic_store(&latest, -1);
        atomic_store(&cur, r);
        for (long i = 0; i < N; ++i) {
            obj* x = malloc(sizeof(obj));
            r->slot[i] = x;
            atomic_store_explicit(&latest, i, memory_order_release);
            x->p = sentinel;         // lazily creates aux: the window is in here
        }
        validate(old[K-1]);
        memmove(&old[1], &old[0], sizeof(ring*) * (K-1));
        old[0] = r;
        rounds++;
    }
    stop = 1;
    for (int i = 0; i < nt2; ++i) pthread_join(th[i], 0);
    for (int i = 0; i < K; ++i) validate(old[i]);
    zprintf("done rounds=%lu\n", rounds);
    return 0;
}
