/* correct program: run each job in a fresh coroutine, contexts are locals */
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int jobs;
static void job(void) { jobs++; }

__attribute__((noinline)) static void run_job(void)
{
    ucontext_t caller, ctx;
    memset(&caller, 0, sizeof caller);
    getcontext(&ctx);
    ctx.uc_stack.ss_size = 65536;
    ctx.uc_stack.ss_sp = malloc(65536);
    ctx.uc_link = &caller;
    makecontext(&ctx, job, 0);
    swapcontext(&caller, &ctx);
    free(ctx.uc_stack.ss_sp);
}

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 2000;
    for (int i = 0; i < n; i++) {
        run_job();
        for (int j = 0; j < 2000; j++) { void * volatile p = malloc(1100); memset(p, 0x41, 1100); }
    }
    printf("ok %d\n", jobs);
    return 0;
}
