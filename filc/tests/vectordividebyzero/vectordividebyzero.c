#include <stdfil.h>

typedef int int4 __attribute__((vector_size(16)));

int main()
{
    int4 x = { 10, 1, 7, 9 };
    int4 y = { 2, 0, 7, 3 };
    asm("" : "+x"(x), "+x"(y));
    int4 q = x / y;
    zprintf("(1) %d %d %d %d\n", q[0], q[1], q[2], q[3]);
    int4 r = x % y;
    zprintf("(2) %d %d %d %d\n", r[0], r[1], r[2], r[3]);
    x = (int4){ 10, 1, (int)0x80000000, 9 };
    y = (int4){ 2, 1, -1, 3 };
    asm("" : "+x"(x), "+x"(y));
    q = x / y;
    zprintf("(3) %d %d %d %d\n", q[0], q[1], q[2], q[3]);
    r = x % y;
    zprintf("(4) %d %d %d %d\n", r[0], r[1], r[2], r[3]);
    return 0;
}
