#include <stdfil.h>
#include <setjmp.h>
#include <inttypes.h>
#include <stdlib.h>
#include <filc_test_support.h>

static unsigned n;
static jmp_buf* buf;
static zweak** weaks;
static uintptr_t* intvals;
static unsigned i;

static __attribute__((noinline)) void init(void)
{
    n = 2;
    buf = zgc_alloc(sizeof(jmp_buf) * n);
    weaks = zgc_alloc(sizeof(zweak*) * n);
    intvals = zgc_alloc(sizeof(uintptr_t) * n);
}

static __attribute__((noinline)) int* make(unsigned i)
{
    int* ptr = zgc_alloc(16);
    zprintf("ptr = %P\n", ptr);
    weaks[i] = zweak_new(ptr);
    intvals[i] = (uintptr_t)ptr;
    *ptr = 1410;
    return ptr;
}

static __attribute__((noinline)) void gc(void)
{
    zprintf("gcing...\n");
    zmemset(buf, 0, sizeof(jmp_buf) * n);
    zgc_request_and_wait();
    zprintf("done gcing.\n");
}

static __attribute__((noinline)) void check(uintptr_t intval)
{
    zprintf("ptr = %p\n", (void*)intval);
    for (unsigned j = 0; j < n; ++j) {
        if (intvals[j] == intval) {
            ZASSERT((uintptr_t)zweak_get(weaks[j]) == intval);
            zprintf("succeeded with j = %u, ptr = %p\n", j, (void*)intval);
            return;
        }
    }
    ZASSERT(!"Should not be reached");
}

int main()
{
    init();
    for (i = 0; i < n; ++i) {
        int* ptr = make(i);
        zhas_union(&ptr);
        if (setjmp(buf + i)) {
            gc();
            check((uintptr_t)zgetlower(ptr));
            return 0;
        }
        if (i == n - 1)
            longjmp(buf, 1);
    }
    ZASSERT(!"Should not be reached");
    return 0;
}
