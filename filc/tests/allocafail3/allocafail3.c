#include <stdlib.h>
#include <signal.h>
#include <stdfil.h>
#include <filc_test_support.h>
#include <stdbool.h>

static int* gptr;
static bool ran;

static void handler(int signo)
{
    zgc_free(gptr);
    zgc_request_and_wait();
    ran = true;
}

static __attribute__((noinline)) void foo(volatile int *ptr2)
{
    bool yes = true;
    asm ("" : "+r"(yes));
    if (!yes)
        return;
    *ptr2 = 42;
    int* ptr3 = alloca(sizeof(int) * 1000000);
    asm volatile("" : : "r"(*ptr2), "r"(ptr3) : "memory");
}

int main()
{
    for (unsigned count = 1000; count--;) {
        int* ptr = zgc_alloc(sizeof(int) * 1000000);
        *ptr = 42;
    }
    zgc_request_and_wait();
    int* ptr2 = zgc_alloc(sizeof(int));
    gptr = ptr2;
    signal(SIGUSR1, handler);
    zsys_raise_without_exit(SIGUSR1);
    foo(ptr2);
    ZASSERT(!ran);
    return 0;
}


