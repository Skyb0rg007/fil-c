#include <stdfil.h>

int main()
{
    int x = 1;
    int y = 0;
    asm("" : "+r"(x), "+r"(y));
    zprintf("(1) %d\n", x / y);
    zprintf("(2) %d\n", x % y);
    x = 0x80000000;
    y = -1;
    asm("" : "+r"(x), "+r"(y));
    zprintf("(3) %d\n", x / y);
    zprintf("(4) %d\n", x % y);
    return 0;
}


