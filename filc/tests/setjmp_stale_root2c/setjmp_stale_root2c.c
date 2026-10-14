#include <stdfil.h>
#include <setjmp.h>
#include <inttypes.h>
#include <stdlib.h>

int main()
{
    unsigned n = 2;
    asm("" : "+r"(n));
    jmp_buf* buf = zgc_alloc(sizeof(jmp_buf) * n);
    zweak** weaks = zgc_alloc(sizeof(zweak*) * n);
    uintptr_t* intvals = zgc_alloc(sizeof(uintptr_t) * n);
    for (unsigned i = 0; i < n; ++i) {
        int* ptr = zgc_alloc(16);
        weaks[i] = zweak_new(ptr);
        intvals[i] = (uintptr_t)ptr;
        *ptr = 1410;
        if (setjmp(buf + i)) {
            zmemset(buf, 0, sizeof(jmp_buf) * n);
            zgc_request_and_wait();
            for (unsigned j = 0; j < n; ++j) {
                if (intvals[j] == (uintptr_t)ptr) {
                    ZASSERT(zweak_get(weaks[j]) == ptr);
                    return 0;
                }
            }
            ZASSERT(!"Should not be reached");
        }
        if (i == n - 1)
            longjmp(buf, 1);
    }
    ZASSERT(!"Should not be reached");
    return 0;
}
