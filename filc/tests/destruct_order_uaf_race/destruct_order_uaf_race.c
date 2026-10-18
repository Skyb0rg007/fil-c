/* Destruct-order UAF: context C saved on fiber A's native stack (C.stack_owner = A).  When both die in the same GC
   cycle, A's destructor bmalloc_deallocate()s the stack, then C's destructor walks C->top_native_frame, which lives
   in that freed stack.  The mutator meanwhile re-allocates the memory as a new fiber stack and scribbles it. */
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define N 1500
#define SS 16384
static ucontext_t mainctx;
static ucontext_t *A[N], *B[N];
static void *dummy_stack;
static int cur;

__attribute__((noinline)) static unsigned long fill(int n, unsigned long v)
{
    volatile unsigned long pad[64];
    for (int i = 0; i < 64; i++) pad[i] = v;
    unsigned long r = pad[n & 63];
    if (n) r += fill(n - 1, v);
    return r + pad[(n+1)&63];
}
static ucontext_t *mk(void (*fn)(void))
{
    ucontext_t *u = calloc(1, sizeof *u);
    getcontext(u);
    u->uc_stack.ss_sp = malloc(SS); u->uc_stack.ss_size = SS;
    makecontext(u, fn, 0);
    return u;
}
static void inner(void) { setcontext(&mainctx); }         /* generator abandoned: goes straight back to main */
static void outer(void)
{
    ucontext_t me;                                         /* C: fresh context saved on A's stack */
    memset(&me, 0, sizeof me);
    swapcontext(&me, B[cur]);
    abort();
}
static void scribbler(void) { fill(20, 0x4141414141414141ul); setcontext(&mainctx); }

int main(int argc, char **argv)
{
    int rounds = argc > 1 ? atoi(argv[1]) : 30;
    dummy_stack = malloc(SS);
    for (int r = 0; r < rounds; r++) {
        for (int i = 0; i < N; i++) A[i] = mk(outer);
        for (int i = 0; i < N; i++) B[i] = mk(inner);
        for (cur = 0; cur < N; cur++) swapcontext(&mainctx, A[cur]);
        for (int i = 0; i < N; i++) A[i] = B[i] = NULL;
        /* keep making and running short-lived fibers while the GC destructs the batch above */
        for (int i = 0; i < 4 * N; i++) { ucontext_t *s = mk(scribbler); swapcontext(&mainctx, s); }
    }
    puts("done");
    return 0;
}
