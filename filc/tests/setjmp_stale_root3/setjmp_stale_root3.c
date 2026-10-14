#include <stdfil.h>
#include <setjmp.h>
#include <inttypes.h>

#ifdef __x86_64__
#define CLOBBER() asm volatile("" : : : "rax", "rcx", "rdx", "rbx", "rsi", "rdi", \
                               "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15")
#elif defined(__aarch64__)
#define CLOBBER() asm volatile("" : : : "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7", \
                               "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15", \
                               "x16", "x17", "x18", "x20", "x21", "x22", "x23", "x24", \
                               "x25", "x26", "x27", "x28", "x30")
#else
#error "This test doesn't support your CPU."
#endif

int main()
{
    unsigned n = 2;
    asm("" : "+r"(n));
    jmp_buf* buf = zgc_alloc(sizeof(jmp_buf) * n);
    zweak** weaks = zgc_alloc(sizeof(zweak*) * n);
    uintptr_t* intvals = zgc_alloc(sizeof(uintptr_t) * n);
    for (unsigned i = 0; i < n; ++i) {
        int* ptr = zgc_alloc(16);
        zprintf("ptr = %P\n", ptr);
        CLOBBER();
        weaks[i] = zweak_new(ptr);
        intvals[i] = (uintptr_t)ptr;
        *ptr = 1410;
        CLOBBER();
        if (setjmp(buf + i)) {
            CLOBBER();
            zmemset(buf, 0, sizeof(jmp_buf) * n);
            zgc_request_and_wait();
            for (unsigned j = 0; j < n; ++j) {
                CLOBBER();
                if (intvals[j] == (uintptr_t)ptr) {
                    ZASSERT(zweak_get(weaks[j]) == ptr);
                    zprintf("succeeded with j = %u, ptr = %P\n", j, ptr);
                    return 0;
                }
            }
            ZASSERT(!"Should not be reached");
        }
        CLOBBER();
        if (i == n - 1)
            longjmp(buf, 1);
    }
    ZASSERT(!"Should not be reached");
    return 0;
}
