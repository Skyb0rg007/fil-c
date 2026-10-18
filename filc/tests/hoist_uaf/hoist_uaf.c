// NotFree/bounds check on `s` is hoisted above memmove(); memmove contains pollchecks, so another
// thread can free(s) and a whole GC cycle can complete while we're in memmove. GC does not mark
// through a freed object => s->q's target is swept, but we still load s->q's capability afterwards.
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdfil.h>

struct S { unsigned long* q; };
static struct S* volatile gs;
static volatile int go, done;
#define NSPRAY 20000
static unsigned long* spray[NSPRAY];

__attribute__((noinline)) unsigned long f(struct S* s, char* d, char* src, size_t n) {
    go = 1;
    memmove(d, src, n);
    return s->q[1];         // no free check after the memmove
}

static void* thr(void* arg) {
    while (!go) ;
    free(gs);               // the program's bug: racing free
    gs = 0;
    zgc_request_and_wait();
    zgc_request_and_wait();
    for (int i = 0; i < NSPRAY; i++) {
        spray[i] = malloc(64);
        for (int j = 0; j < 8; j++) spray[i][j] = 0x5ec2e75ec2e70000ul + i;
    }
    done = 1;
    return 0;
}

int main(int argc, char** argv) {
    size_t n = (size_t)(argc > 1 ? atol(argv[1]) : 512) << 20;
    char* d = malloc(n); char* src = malloc(n);
    // some same-size-class neighbours so that the page stays in use
    struct S* s = malloc(sizeof *s);
    s->q = malloc(64);
    for (int j = 0; j < 8; j++) s->q[j] = 0x1111111111111111ul;
    gs = s;
    pthread_t t; pthread_create(&t, 0, thr, 0);
    unsigned long v = f(s, d, src, n);
    printf("done=%d  read through stale s->q: %lx\n", done, v);
    ZASSERT(v == 0x1111111111111111ul);
    pthread_join(t, 0);
    return 0;
}
