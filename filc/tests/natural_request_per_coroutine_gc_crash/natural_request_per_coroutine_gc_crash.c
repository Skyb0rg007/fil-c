/* un-tuned natural shape: every request is served by a fresh coroutine; both ucontext_t are locals of serve().
   Correct program, no Fil-C API, natural GC only. */
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct node { struct node *next; unsigned long tag; char pad[48]; };
static ucontext_t *cur_caller, *cur_ctx;
static long bad, reqs;
__attribute__((noinline)) static void check(struct node *l, unsigned long tag, int n)
{
    int c = 0;
    for (; l && c <= n; l = l->next, c++)
        if (l->tag != tag) { bad++; printf("request %ld: live node %p corrupted: tag=%lx expected %lx\n", reqs, l, l->tag, tag); return; }
    if (c != n) { bad++; printf("request %ld: list length %d != %d\n", reqs, c, n); }
}
static void handler(void)
{
    unsigned long tag = 0xC0FFEE0000ul + reqs;
    struct node *l = NULL;
    for (int i = 0; i < 20; i++) { struct node *x = malloc(sizeof *x); x->tag = tag; x->next = l; l = x; }
    swapcontext(cur_ctx, cur_caller);          /* "wait for I/O" */
    check(l, tag, 20);
}
__attribute__((noinline)) static void churn(int n) { for (int j = 0; j < n; j++) { char * volatile p = malloc(64); memset(p, 0x41, 64); } }
__attribute__((noinline)) static void serve(int nested)
{
    ucontext_t caller, ctx;
    memset(&caller, 0, sizeof caller);
    getcontext(&ctx);
    ctx.uc_stack.ss_size = 65536;
    ctx.uc_stack.ss_sp = malloc(65536);
    ctx.uc_link = &caller;
    makecontext(&ctx, handler, 0);
    cur_caller = &caller; cur_ctx = &ctx;
    swapcontext(&caller, &ctx);
    churn(30000);                               /* other work while the request waits */
    swapcontext(&caller, &ctx);
    cur_caller = cur_ctx = NULL;
    reqs++;
}
int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 3000;
    for (int i = 0; i < n && !bad; i++) { serve(0); churn((i * 7919) % 40000); }
    printf("%ld requests, %ld corrupted\n", reqs, bad);
    return 0;
}
