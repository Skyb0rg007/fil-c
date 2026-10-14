#include <stdfil.h>
#include <setjmp.h>
#include <inttypes.h>
#include <stdlib.h>

static jmp_buf* buf;
static zweak** weaks;
static uintptr_t* intvals;

static void test(zweak* weak_ptr)
{
    zmemset(buf, 0, sizeof(jmp_buf) * 2);
    zgc_request_and_wait();
    for (unsigned j = 0; j < 2; ++j) {
        ZASSERT(zweak_get(weak_ptr));
        if (intvals[j] == (uintptr_t)zweak_get(weak_ptr)) {
            ZASSERT(zweak_get(weaks[j]) == zweak_get(weak_ptr));
            return;
        }
    }
    ZASSERT(!"Should not be reached");
}

int main()
{
    buf = zgc_alloc(sizeof(jmp_buf) * 2);
    weaks = zgc_alloc(sizeof(zweak*) * 2);
    intvals = zgc_alloc(sizeof(uintptr_t) * 2);
    for (unsigned i = 0; i < 2; ++i) {
        int* ptr = zgc_alloc(16);
        weaks[i] = zweak_new(ptr);
        intvals[i] = (uintptr_t)ptr;
        *ptr = 1410;
        if (setjmp(buf + i)) {
            test(zweak_new(ptr));
            asm volatile ("" : : "r"(ptr) : "memory");
            return 0;
        }
        if (i == 2 - 1)
            longjmp(buf, 1);
    }
    ZASSERT(!"Should not be reached");
    return 0;
}
