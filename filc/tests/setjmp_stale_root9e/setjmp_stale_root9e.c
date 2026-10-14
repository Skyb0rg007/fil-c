#include <stdfil.h>
#include <setjmp.h>
#include <inttypes.h>
#include <stdlib.h>

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
    zprintf("buf = %P\n", buf);
    zprintf("weaks = %P\n", weaks);
    zprintf("intvals = %P\n", intvals);
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

static __attribute__((noinline)) void jmp(void)
{
    zmemset(buf + 1, 0, sizeof(jmp_buf) * (n - 1));
    zprintf("gcing before jmp...\n");
    zdump_heap(1);
    zgc_request_and_wait();
    zprintf("done gcing before jmp.\n");
    longjmp(buf, 1);
}

int main()
{
    init();
    for (i = 0; i < n; ++i) {
        int* ptr = make(i);
        if (setjmp(buf + i)) {
            gc();
            check((uintptr_t)zgetlower(ptr));
            return 0;
        }
        int *a = zgc_alloc(16);
        int *b = zgc_alloc(16);
        int *c = zgc_alloc(16);
        int *d = zgc_alloc(16);
        int *e = zgc_alloc(16);
        int *f = zgc_alloc(16);
        int *g = zgc_alloc(16);
        int *h = zgc_alloc(16);
        int *I = zgc_alloc(16);
        int *j = zgc_alloc(16);
        int *k = zgc_alloc(16);
        int *l = zgc_alloc(16);
        int *m = zgc_alloc(16);
        int *N = zgc_alloc(16);
        int *o = zgc_alloc(16);
        int *p = zgc_alloc(16);
        int *q = zgc_alloc(16);
        int *r = zgc_alloc(16);
        asm volatile ("" : "+r"(a) : : "memory");
        asm volatile ("" : "+r"(b) : : "memory");
        asm volatile ("" : "+r"(c) : : "memory");
        asm volatile ("" : "+r"(d) : : "memory");
        asm volatile ("" : "+r"(e) : : "memory");
        asm volatile ("" : "+r"(f) : : "memory");
        asm volatile ("" : "+r"(g) : : "memory");
        asm volatile ("" : "+r"(h) : : "memory");
        asm volatile ("" : "+r"(I) : : "memory");
        asm volatile ("" : "+r"(j) : : "memory");
        asm volatile ("" : "+r"(k) : : "memory");
        asm volatile ("" : "+r"(l) : : "memory");
        asm volatile ("" : "+r"(m) : : "memory");
        asm volatile ("" : "+r"(N) : : "memory");
        asm volatile ("" : "+r"(o) : : "memory");
        asm volatile ("" : "+r"(p) : : "memory");
        asm volatile ("" : "+r"(q) : : "memory");
        asm volatile ("" : "+r"(r) : : "memory");
        if (i == n - 1)
            jmp();
    }
    ZASSERT(!"Should not be reached");
    return 0;
}
