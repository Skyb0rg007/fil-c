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

int main()
{
    for (unsigned count = 1000; count--;) {
        int* ptr = zgc_alloc(sizeof(int) * 1000000);
        *ptr = 42;
    }
    zgc_request_and_wait();
    volatile int* ptr2 = zgc_alloc(sizeof(int));
    gptr = ptr2;
    signal(SIGUSR1, handler);
    zsys_raise_without_exit(SIGUSR1);
    *ptr2 = 42;
    int* ptr3 = alloca(sizeof(int) * 1000000);
    asm volatile("" : : "r"(*ptr2), "r"(ptr3) : "memory");
    ZASSERT(!ran);
    return 0;
}


