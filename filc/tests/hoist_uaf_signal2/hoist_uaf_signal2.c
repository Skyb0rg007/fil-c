// Single-threaded variant: the free happens in a signal handler, which Fil-C runs at a pollcheck
// inside memmove()/memset()/large malloc(). Select the window with argv[1]: 0=memmove 1=memset 2=malloc
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <stdfil.h>
#include <filc_test_support.h>

struct S { unsigned long* q; };
static struct S* gs;
static volatile int done;
#define NSPRAY 2000
static unsigned long* spray[NSPRAY];

__attribute__((noinline)) unsigned long f0(struct S* s, char* d, char* src, size_t n) {
    memmove(d, src, n);
    return s->q[1];
}
__attribute__((noinline)) unsigned long f1(struct S* s, char* d, char* src, size_t n) {
    memset(d, 1, n);
    return s->q[1];
}
char* volatile sink;
__attribute__((noinline)) unsigned long f2(struct S* s, char* d, char* src, size_t n) {
    sink = malloc(n);
    return s->q[1];
}

static void handler(int sig) {
    if (done) return;
    free(gs); gs = 0;
    zgc_request_and_wait();
    zgc_request_and_wait();
    for (int i = 0; i < NSPRAY; i++) {
        spray[i] = malloc(64);
        for (int j = 0; j < 8; j++) spray[i][j] = 0x5ec2e75ec2e70000ul + i;
    }
    done = 1;
}
__attribute__((noinline)) static struct S* mk(void) {
    struct S* s = malloc(sizeof *s);
    s->q = malloc(64);
    for (int j = 0; j < 8; j++) s->q[j] = 0x1111111111111111ul;
    return s;
}
int thingy(int mode, size_t n) {
    done = 0;
    char* d = malloc(n); char* src = malloc(n);
    if (mode == 2) { memset(d, 1, n); free(d); d = 0; zgc_request_and_wait(); } // make malloc need zeroing
    struct S* s = mk();
    gs = s;
    signal(SIGALRM, handler);
    zsys_raise_without_exit(SIGALRM);
    unsigned long v = mode == 0 ? f0(s, d, src, n) : mode == 1 ? f1(s, d, src, n) : f2(s, d, src, n);
    printf("done=%d mode=%d  read through stale s->q: %lx\n", done, mode, v);
    ZASSERT(v == 0x1111111111111111ul);
    return 0;
}
int main()
{
    thingy(0, 512 << 20);
    thingy(1, 512 << 20);
    thingy(2, 512 << 20);
}
