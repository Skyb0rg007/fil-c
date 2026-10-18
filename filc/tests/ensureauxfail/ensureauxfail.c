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
        int** ptr = zgc_alloc(sizeof(int*) * 1000000);
        *ptr = zgc_alloc(16);
    }
    zgc_request_and_wait();
    int** ptr = zgc_alloc(sizeof(int*) * 1000000);
    int* ptr2 = zgc_alloc(sizeof(int));
    gptr = ptr2;
    signal(SIGUSR1, handler);
    zsys_raise_without_exit(SIGUSR1);
    *ptr = zgc_alloc(16);
    asm volatile("" : : "r"(*ptr2));
    ZASSERT(!ran);
    return 0;
}


