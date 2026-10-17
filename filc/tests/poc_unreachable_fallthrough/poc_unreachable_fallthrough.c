#include <stdio.h>
#include <stdlib.h>
int arr[16];
__attribute__((noinline)) int get(void) { return 33; }
__attribute__((noinline)) void f(unsigned v) {
    if ((v << get()) > 5) { puts("a"); arr[3] = v; } else { printf("b %d\n", v); arr[4] = 5; }
}
__attribute__((noinline)) void g(int v) {
    puts("in g");
}
int main(int argc, char**argv) { f(argc); printf("done %d\n", arr[0]); if (argc > 5) g(1); return 0; }
