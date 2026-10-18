// CORRECT POSIX program (no free, no UB): one ucontext_t is re-initialised with getcontext()/makecontext() to create the next fiber
// while we are running on the fiber that was started from it with setcontext(). getcontext() overwrites uc->__uc_fiber_context, the
// only heap reference to the *running* filc_fiber_context; thread->current_fiber_context is not a GC root. If a GC cycle completes
// before the swapcontext (here forced; naturally: malloc(65536) + closure + zargs allocations are in the window), the running native stack
// is bmalloc_deallocate()d and handed to the new fiber by makecontext().
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdfil.h>

static ucontext_t launch, self, other_self;
static volatile unsigned idx;
static int depth_arg = 200;

static void start(void (*f)(void))
{
    getcontext(&launch);
    launch.uc_stack.ss_sp = malloc(65536);
    launch.uc_stack.ss_size = 65536;
    launch.uc_link = NULL;
    zgc_request_and_wait();     /* stand-in for a GC triggered by allocation */
    makecontext(&launch, f, 0);
}

static void fn2(void)
{
    long a[1024];                 /* non-escaping => lives on the native stack */
    for (unsigned i = 0; i < 1024; i++) a[i] = 0x4141414141414141L;
    idx = (unsigned)a[idx & 1023] & 1023;
    swapcontext(&other_self, &self);
}

__attribute__((noinline)) static void inner(int depth)
{
    if (depth) { inner(depth - 1); idx++; return; }
    start(fn2);
    zprintf("switching to fn2\n");
    swapcontext(&self, &launch);
    zprintf("back in fn\n");
}

static void fn(void)
{
    inner(depth_arg);
    zprintf("fiber survived\n");
    exit(0);
}

int main(int argc, char** argv)
{
    if (argc > 1) depth_arg = atoi(argv[1]);
    start(fn);
    setcontext(&launch);
    return 1;
}
